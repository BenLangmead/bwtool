/*****
 ** kmeans.c
 ** - a simple k-means clustering routine
 ** - returns the cluster labels of the data points in an array
 ** - here's an example
 **   extern int *k_means(double**, int, int, int, double, double**);
 **   ...
 **   int *c = k_means(data_points, num_points, dim, 20, 1e-4, 0);
 **   for (i = 0; i < num_points; i++) {
 **      printf("data point %d is in cluster %d\n", i, c[i]);
 **   }
 **   ...
 **   free(c);
 ** Parameters
 ** - array of data points (double **data)
 ** - number of data points (int n)
 ** - dimension (int m)
 ** - desired number of clusters (int k)
 ** - error tolerance (double t)
 **   - used as the stopping criterion, i.e. when the sum of
 **     squared euclidean distance (standard error for k-means)
 **     of an iteration is within the tolerable range from that
 **     of the previous iteration, the clusters are considered
 **     "stable", and the function returns
 **   - a suggested value would be 0.0001
 ** - output address for the final centroids (double **centroids)
 **   - user must make sure the memory is properly allocated, or
 **     pass the null pointer if not interested in the centroids
 ** References
 ** - J. MacQueen, "Some methods for classification and analysis
 **   of multivariate observations", Fifth Berkeley Symposium on
 **   Math Statistics and Probability, 281-297, 1967.
 ** - I.S. Dhillon and D.S. Modha, "A data-clustering algorithm
 **   on distributed memory multiprocessors",
 **   Large-Scale Parallel Data Mining, 245-260, 1999.
 ** Notes
 ** - this function is provided as is with no warranty.
 ** - the author is not responsible for any damage caused
 **   either directly or indirectly by using this function.
 ** - anybody is free to do whatever he/she wants with this
 **   function as long as this header section is preserved.
 ** Created on 2005-04-12 by
 ** - Roger Zhang (rogerz@cs.dal.ca)
 ** Modifications
 ** -
 ** Last compiled under Linux with gcc-3
 */

#include "common.h"
#include "obscure.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include "linefile.h"
#include "hash.h"
#include "options.h"
#include "sqlNum.h"
#include "basicBed.h"
#include "bigWig.h"
#include "metaBig.h"
#include "bigs.h"
#include "cluster.h"

/* static void perBaseMatrixDump(const struct perBaseMatrix *pbm) */
/* /\* for debugging *\/ */
/* { */
/*     int i, j; */
/*     for (i = 0; i < pbm->nrow; i++) */
/*     { */
/* 	for (j = 0; j < pbm->ncol; j++) */
/* 	    uglyf("\t%0.2f", pbm->matrix[i][j]); */
/* 	uglyf("\n"); */
/*     } */
/* } */

static int perBaseWigJustLabelCmp(const void *a, const void *b)
/* for sorting after clustering */
{
    const struct perBaseWig *pbw_a = *((struct perBaseWig **)a);
    const struct perBaseWig *pbw_b = *((struct perBaseWig **)b);    
    int diff = pbw_a->label - pbw_b->label;
    return diff;
}

static int clear_na_rows(struct perBaseMatrix *pbm)
/* if an NA is encountered in the matrix row, set its label to -1 */
/* and put it at the beginning. return the number of rows like this */
{
    int i, j;
    int num_na = 0;
    for (i = 0; i < pbm->nrow; i++)
    {
	for (j = 0; j < pbm->ncol; j++)
	{
	    if (isnan(pbm->matrix[i][j]))
	    {
		pbm->array[i]->label = -1;
		num_na++;
		break;
	    }
	}
    }
    qsort(pbm->array, pbm->nrow, sizeof(pbm->array[0]), perBaseWigJustLabelCmp);
    for (i = 0; i < pbm->nrow; i++)
	pbm->matrix[i] = pbm->array[i]->data;
    return num_na;
}

struct cluster_bed_matrix *init_cbm_from_pbm(struct perBaseMatrix *pbm, int k)
/* initialize the cluster struct froma matrix */
{
    struct cluster_bed_matrix *cbm;
    AllocVar(cbm);
    cbm->pbm = pbm;
    cbm->m = pbm->ncol;
    cbm->n = pbm->nrow;
    cbm->k = k;
    cbm->num_na = clear_na_rows(cbm->pbm);
    return cbm;
}

struct cluster_bed_matrix *init_cbm(struct metaBig *mb, struct bed6 *regions, int k)
/* initialize the cluster struct */
{
    struct perBaseMatrix *pbm = load_perBaseMatrix(mb, regions);
    return init_cbm_from_pbm(pbm, k);
}

void free_cbm(struct cluster_bed_matrix **pCbm)
/* free up the cluster struct */
{
    struct cluster_bed_matrix *cbm = *pCbm;
    free_perBaseMatrix(&cbm->pbm);
    freeMem(cbm->cluster_sizes);
    freeMem(cbm->centroids);
    freez(&cbm);
}

static int *k_means(struct cluster_bed_matrix *cbm, double t)
{
    /* output cluster label for each data point */
    int *labels; /* Labels for each cluster (size n) */
    int h, i, j; /* loop counters, of course :) */
    double old_error; 
    double error = DBL_MAX; /* sum of squared euclidean distance */
    double **c1; /* centroids and temp centroids (size k x m) */
    int n = cbm->n;
    int m = cbm->m;
    int k = cbm->k;
    AllocArray(labels, n);
    AllocArray(cbm->cluster_sizes, k);
    AllocArray(cbm->centroids, k);
    AllocArray(c1, k);
    for (i = 0; i < k; i++)
    {
	AllocArray(cbm->centroids[i], m);
	AllocArray(c1[i], m);
    }
    /* assert(data && k > 0 && k <= n && m > 0 && t >= 0); /\* for debugging *\/ */
    /* init ialization */
    for (i = 0, h = cbm->num_na; i < k; h += (cbm->n-cbm->num_na) / k, i++) 
    {
	/* pick k points as initial centroids */
	for (j = cbm->m-1; j >= 0; j--)
	    cbm->centroids[i][j] = cbm->pbm->matrix[h][j];
    }
    /* main loop */
    do 
    {
	/* save error from last step */
	old_error = error;
	error = 0;
	/* clear old cbm->cluster_sizes and temp centroids */
	for (i = 0; i < k; i++)
	{
	    cbm->cluster_sizes[i] = 0;
	    for (j = 0; j < m; j++)
		c1[i][j] = 0;
	}
	for (h = cbm->num_na; h < n; h++) 
	{
	    /* identify the closest cluster */
	    double min_distance = DBL_MAX;
	    for (i = 0; i < k; i++) 
	    {
		double distance = 0;
		for (j = m-1; j >= 0; j--)
			distance += pow(cbm->pbm->matrix[h][j] - cbm->centroids[i][j], 2);
		if (distance < min_distance) 
		{
		    labels[h] = i;
		    min_distance = distance;
		}
	    }
	    /* update size and temp centroid of the destination cluster */
	    for (j = m-1; j >= 0; j--)
		c1[labels[h]][j] += cbm->pbm->matrix[h][j];
	    cbm->cluster_sizes[labels[h]]++;
	    /* update standard error */
	    error += min_distance;
	}
	for (i = 0; i < k; i++) 
	{ /* update all centroids */
	    for (j = 0; j < m; j++) 
	    {
		cbm->centroids[i][j] = cbm->cluster_sizes[i] ? (c1[i][j] / cbm->cluster_sizes[i]) : c1[i][j];
	    }
	}
    } while (fabs(error - old_error) > t);
    /* housekeeping */
    freeMem(c1);
    return labels;
}
    
void do_kmeans(struct cluster_bed_matrix *cbm, double t)
/* the main clustering function.  labels matrix rows and reorders */
{
    int i = 0;
    int *labels = k_means(cbm, t);
    for (i = cbm->num_na; i < cbm->pbm->nrow; i++)
	cbm->pbm->array[i]->label = labels[i];
    qsort(cbm->pbm->array, cbm->pbm->nrow, sizeof(cbm->pbm->array[0]), perBaseWigLabelCmp);
    for (i = 0; i < cbm->pbm->nrow; i++)
	cbm->pbm->matrix[i] = cbm->pbm->array[i]->data;
    freeMem(labels);
}