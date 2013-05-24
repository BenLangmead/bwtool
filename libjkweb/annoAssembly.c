/* annoAssembly -- basic metadata about an assembly for the annoGrator framework. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "twoBit.h"
#include "annoAssembly.h"

struct annoAssembly *annoAssemblyNew(char *name, char *twoBitPath)
/* Return an annoAssembly with open twoBitFile. */
{
struct annoAssembly *aa;
AllocVar(aa);
aa->name = cloneString(name);
aa->tbf = twoBitOpen(twoBitPath);
return aa;
}

uint annoAssemblySeqSize(struct annoAssembly *aa, char *seqName)
/* Return the number of bases in seq which must be in aa's twoBitFile. */
{
return (uint)twoBitSeqSize(aa->tbf, seqName);
}

void annoAssemblyClose(struct annoAssembly **pAa)
/* Close aa's twoBitFile and free mem. */
{
if (*pAa == NULL)
    return;
struct annoAssembly *aa = *pAa;
freeMem(aa->name);
twoBitClose(&(aa->tbf));
freez(pAa);
}

