extern outlinks(char *file)
{
	int i=0,flag=1;
	FILE *fp,*fpt,*dt;
	char *str1,*temp,*str;
	str1=(char *)malloc(100);
	str=(char *)malloc(100);
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
			/* seearch for this string 'temp'(url) in our dataset
			and print it in outlinks file oly if its present in dataset*/

			dt=fopen("url.txt","r");
			while(flag)
			{
				if(fscanf(dt,"%s",str)!=EOF)
				{
					if(strcmp(str,temp)==0)
						flag=0;
				}
				else
					break;
			}
			if(flag==0)
			{
				fprintf(fpt,"%s\n",temp);
				printf("%s\n",temp);
				flag=1;
			}
			fclose(dt);
		      i=0;
		}

	}
	fclose(fp);
	fclose(fpt);
}
