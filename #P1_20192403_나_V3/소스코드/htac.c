#include "types.h"
#include "stat.h"
#include "user.h"

char *buf;
int line;

//파일의 끝까지 읽어 파일의 전체 크기를 가져오는 함수
int
retFileSize(int fd)
{
  int ret=0,n;
  char tempBuf[512];
  while((n = read(fd, tempBuf, sizeof(tempBuf))) > 0) {
    ret+=n;
  }
  if(n<0) {
	printf(1, "cat: read error\n");
	exit();
  }
  return ret;
}

// 파일의 전체 크기를 받아 한번에 파일을 읽어들인 후 개행을 찾아 한 줄씩 출력
void
cat(int fd, int fileSize)
{
  int n,cnt=0,end_line;
  buf = malloc(fileSize+1);
  n = read(fd, buf, fileSize);
  if(n != fileSize){
    printf(1, "cat: read error\n");
    exit();
  }
  end_line = n;
  for(int i=n-1; i >= 0; i--) {
	if(buf[i] == '\n') {
	  write(1, &buf[i+1],end_line-(i+1));
	  write(1, "\n", 1);
	  end_line = i;
	  cnt++;
	}
	if(cnt == line) break;
  }
  free(buf);
}

int
main(int argc, char *argv[])
{
  int fd, i, fileSize;

  if(argc <= 2){
    cat(0,0);
    exit();
  }
  line  = atoi(argv[1]);
  if(line < 0) {
	  printf(1, "Lines can only be positive.\n");
	  exit();
  }
  for(i = argc-1; i >= 2; i--){
	if((fd = open(argv[i], 0)) < 0){
      printf(1, "cat: cannot open %s\n", argv[i]);
      exit();
    }
    fileSize = retFileSize(fd);
    close(fd);
    if((fd = open(argv[i], 0)) < 0){
      printf(1, "cat: cannot open %s\n", argv[i]);
      exit();
    }
    cat(fd, fileSize);
    close(fd);
  }
  exit();
}
