#define	TRUE	1
#define	FALSE	0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/errno.h>

extern int encode_uri(char *src, char *dest);
extern int decode_uri(char *src, char *dest);

struct stat sb;
char *buf;

int nlines;
char **lines;
int nwords;
int nuniqs;
char  *wbuff;
char **words;
char **uniqs;


int
compar(const void *a, const void *b)
{
  char **sa = (char **) a;
  char **sb = (char **) b;
  return(strcmp(*sa, *sb));
}


int
ReadTriplets(const char *triplets)
{
  int fd = open(triplets, O_RDONLY);
  int ret = fstat(fd, &sb);
  if (ret != 0) {
    printf("%d -- %s\n", errno, strerror(errno));
    close(fd);
    return(-1);
  }

  buf = (char *) malloc(sb.st_size+1);
  bzero(buf, sb.st_size+1);
  read(fd, buf, sb.st_size);
  close(fd);

  int i = 0;
  int j = 0;
  int n = 0;
  char *o, *p, *q;

  for (p = buf; p != &buf[sb.st_size]; p++) {
    if (*p == '\n') n++;
  }

  lines = (char **) calloc(n, sizeof(char *));
  nlines = 0;

  for (p = o = buf; p != &buf[sb.st_size]; p++) {
    if (*p == '\n') {
      *p = '\0';
      lines[nlines++] = o;
      o = p+1;
    }
  }

  // Wikipediaのページタイトルは255バイトが上限
  wbuff = (char *) calloc(nlines*3, 256);
  words = (char **) calloc(nlines*3, sizeof(char *));
  nwords = 0;
  n = nlines*3*256;
  for (i = 0, j = 0; i < n; i += 256, j++) {
    words[j] = &wbuff[i];
  }

  n = 0;
  for (i = 0; i < nlines; i ++) {
    if (strncmp(lines[i], "http", 4) != 0) continue;
    for (p = o = lines[i]; *p != '\0'; p++) {
      if (*p == ' ') {
	*p = '\0';
	for (q = p; *q != '/'; q--){} q++;
	decode_uri(q, words[n]);
	n++;
      }
    }
    for (q = p; *q != '/'; q--){} q++;
    decode_uri(q, words[n]);
    n++;
  }
  nwords = n;

  qsort(words, nwords, sizeof(char *), compar);

  uniqs = (char **) calloc(nwords, sizeof(char *));
  uniqs[0] = words[0];
  n = 0;
  for (i = 1; i < nwords; i++) {
    if (strcmp(uniqs[n], words[i]) == 0) continue;
    uniqs[++n] = words[i];
  }
  nuniqs = n;
#if 0
  for (i = 0; i < nuniqs; i++) {
    printf("%d: %s\n", i, uniqs[i]);
  }    
  printf("nuniqs: %d\n", nuniqs);
  printf("----------\n");
#endif  
  return(0);
}


#define NPAGE (8*1024*1024)

char lbuf[4*1024];
char title[1024];
char page[NPAGE];

int
decode_xml(char *src, char *dest)
{
  int ret = 0;
  char *o, *p;
  for (o = src; *o != '>'; o++) {}
  o++;
  /*
    &lt;	<	
    &gt;	>	
    &amp;	&	アンパサンド
    &quot;	"	ダブルクォーテーション
    &apos;	'	シングルクォーテーション
   */
  for (p = dest; *o != '<'; o++, p++) {
    if (*o == '&') {
      if (o[1] == '#') {
	printf("%s", src);
      } else if (strncmp(o, "&lt;", 4) == 0) {
	*p = '<'; o += 3;
      } else if (strncmp(o, "&gt;", 4) == 0) {
	*p = '>'; o += 3;
      } else if (strncmp(o, "&amp;", 5) == 0) {	
	*p = '&'; o += 4; 
      } else if (strncmp(o, "&quot;", 6) == 0) {	
	*p = '"'; o += 5;
      } else if (strncmp(o, "&apos;", 6) == 0) {	
	*p = '\'';  o += 5;
      }
      ret = 1;
    } else {
      *p = *o;
    }
  }
  return(ret);
}


int tlmax = 21153757;
int tlcnt = 0;

int
ScanWikiData(const char *wikipage)
{
  printf("# wikipage: %s\n", wikipage); fflush(stdout);
  int i;
#if 0
  for (i = 0; i < nuniqs; i++) {
    printf("%d: %s\n", i, uniqs[i]);
  }    
  printf("nuniqs: %d\n", nuniqs);
  printf("----------\n");
#endif  
  FILE *fp = fopen(wikipage, "r");
  if (fp == NULL) {
    printf("%d -- %s\n", errno, strerror(errno));
    return(-1);
  }

  char *lp;
  int visible = TRUE;
  int printit = FALSE;
  for (i = 0;; i++) {
    if ((lp = fgets(lbuf, 4096, fp)) == NULL) break;

    if (strncmp(lp, "  <page>", 8) == 0) {
      visible = FALSE;
      printit = FALSE;
      bzero(page, NPAGE);
    }

    if (visible) {
      printf("%s", lp);
    } else {
      strcat(page, lbuf);
      if (strncmp(lp, "    <title>", 11) == 0) {
	bzero(title, 1024);
	decode_xml(lp, title);
	tlcnt++;
	if ((tlcnt % 10000) == 0) {
	  fprintf(stderr,
		  "%d(%2d%%): %s\n",
		  tlcnt, (tlcnt*100)/tlmax, title);
	}
	// Linear Search
	for (i = 0; i < nuniqs; i++) {
	  if (strcmp(title, uniqs[i]) == 0) {
	    //fprintf(stderr, "%d: %s\n", i, uniqs[i]);
	    printit = TRUE;
	    break;
	  }
	}
      }
    }
    
    if (strncmp(lp, "  </page>", 9) == 0) {
      visible = TRUE;
      if (printit) {
	//printf("%d bytes\n", strlen(page));
	printf("%s", page);  fflush(stdout);
      }
    }

  }
  printf("\n");
  fclose(fp);
  return(0);
}


int
main(int argc, const char * argv[])
{
  ReadTriplets(argv[1]);
  ScanWikiData(argv[2]);
  exit(0);
}
