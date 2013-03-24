#include<stdio.h>
#include<conio.h>
#include"dataset_outlinks.c"
//extern outlinks()
int main()
{
	int i=0,j=0,cnt=0;
	char str[100][15];
	FILE *url;
	clrscr();

	if ((url=fopen("url.txt","r"))==NULL)
	{
		printf("Cannot open file.\n");
		exit(1);
	}
	while(fscanf(url,"%s",str[i])!=EOF)
	{
		if(cnt==1)
		{
			cnt--;
			continue;
		}
		i++;
		cnt++;
	}
	while(j<i)
	{
		printf("\n%s",str[j]);
		j++;
	}
	outlinks(str[0]);

	getch();
}