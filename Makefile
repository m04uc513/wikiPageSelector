PROG=wikiPageSelector
#PROG=ccc

CC=cc -std=c89
CFLAGS=-Wno-format -Wno-return-type

all: ${PROG}

${PROG}: ${PROG}.c uri.c
	${CC} ${CFLAGS} -o ${PROG} ${PROG}.c uri.c

clean:
	rm -f ${PROG}

run:
	./${PROG} ../triplets-data/wikipedia-hand-triplets-release.txt ~/Wikipedia/enwiki-20210420-pages-articles-multistream.xml
