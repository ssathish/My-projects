extern outlinks(char *file)
{
	int i=0;
	FILE *fp,*fpt;
	char *str1,*temp;
	str1=(char *)malloc(100);
	fpt=fopen("out.txt","w");
	if ((fp = fopen(file,"r"))==NULL)
	{
		printf("Cannot open file.\n");
		exit(1);
	}
	/* read an integer from the standard input stream */
	while(fscanf(fp, "%s", str1)!=EOF)
	{

		if((temp=strstr(str1,"href="))!=NULL)
		{

			while(str1[i+6]!='"')
			{
				temp[i]=str1[i+6];
				i++;
			}
			temp[i]=NULL;
			fseek(fpt, 0L, SEEK_END);
			fprintf(fpt,"%s\n",temp);
			printf("%s\n",temp);
		      i=0;
		}

	}
}
