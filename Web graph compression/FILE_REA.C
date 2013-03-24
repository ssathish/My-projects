#include <stdlib.h>
#include <stdio.h>

int main(void)
{
	int i=0;
	FILE *fp,*fpt;
	char *str,*temp;
	clrscr();
	fpt=fopen("url.txt","w");
	if ((fp = fopen("test.txt","r"))==NULL)
	{
		printf("Cannot open file.\n");
		exit(1);
	}
	/* read an integer from the standard input stream */
	while(fscanf(fp, "%s", str)!=EOF)
	{

		if((temp=strstr(str,"href="))!=NULL)
		{

			while(str[i+6]!='"')
			{
				temp[i]=str[i+6];
				i++;
			}
			temp[i]=NULL;
			fseek(fpt, 0L, SEEK_END);
			fprintf(fpt,"%s\n",temp);
			printf("%s\n",temp);
		      i=0;
		}

	}

	getch();
	return 0;
}
