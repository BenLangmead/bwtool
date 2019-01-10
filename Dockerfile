FROM fedora:28

RUN yum -y update && yum -y install gcc gcc-c++ make cmake gdb git zlib-devel bzip2 autoconf
# openssl-devel libpng-devel uuid-devel

RUN git clone https://github.com/CRG-Barcelona/libbeato.git && \
    cd libbeato && \
    git checkout 0c30432af9c7e1e09ba065ad3b2bc042baa54dc2 && \
    ./configure && make && make install

# Workaround for kent tools
#RUN mkdir -p /usr/include/uuid && \
#    ln -s -f /usr/include/uuid.h /usr/include/uuid/uuid.h

#RUN mkdir -p jklib && cd jklib && \
#    curl -L https://genome-source.gi.ucsc.edu/gitlist/kent.git/tarball/master > jklib.tar.gz && \
#    tar xvf jklib.tar.gz && rm -f jklib.tar.gz && \
#    cd src/lib && make && cp x86_64/jkweb.a /usr/local

RUN curl -OL https://github.com/samtools/htslib/releases/download/1.9/htslib-1.9.tar.bz2 && \
    bzip2 -dc htslib-1.9.tar.bz2 | tar xvf - && \ 
    rm -f htslib-1.9.tar.bz2 && \
    cd htslib-1.9 && \
    autoheader && \
    autoconf && \
    ./configure --disable-bz2 --disable-lzma --disable-libcurl --without-libdeflate && \
    make && make install

ADD . /code
WORKDIR /code

RUN mkdir -p build && cd build && cmake .. && make

ENV LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH}"

RUN /code/build/bwtool_langmead || true
RUN ldd -v /code/build/bwtool_langmead
