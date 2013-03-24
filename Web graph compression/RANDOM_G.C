#include<stdio.h>
#include<stdlib.h>       //included to use random function
#include<conio.h>

/******
struct node :: the structure of nodes in d list tat maintains(web sites)
******/

struct node
{
int data;
struct node *link;
struct out_links *list_of_outgoinglinks;
}*root,*curr_node,*next_node;

/******
struct out_links :: the structure of nodes in d list tat maintains all out-going links from a site
******/

struct out_links
{
	struct node *points_to;
	struct out_links *next_node;
}*outlink;

/**********
function prototype declaration
**********/

struct node *create_node();
void graph_gen(int);
struct node *find_node_addr(int);
struct out_links *create_outlink_node(int);
int check(int,int,int *,int *);
void outlink_gen(int,struct node *);

/**************
fun 	: create a new node
param   : nil
return	: address of new_node
***************/

struct node *create_node()
{
	static int node_number=0;
	struct node *new_node;
	new_node=(struct node *)malloc(sizeof(struct node));    //alloc memory
	node_number++;
	new_node->data=node_number;
	new_node->link=NULL;
	new_node->list_of_outgoinglinks=NULL;
	return new_node;
}
/**************
fun 	: generates d graph
param   : total no of nodes
return	: NIL
***************/

void graph_gen(int total_node)
{
	int rem_nodes=total_node;
	root=create_node();
	curr_node=root;
	rem_nodes=rem_nodes-1;
	while(rem_nodes!=0)		//list generation
	{
		next_node=create_node();
		curr_node->link=next_node;
		curr_node=next_node;
		rem_nodes=rem_nodes-1;
	}                              //while end

/********
traversing the list
********/

	curr_node=root;
	printf(" %d ",curr_node->data);
	do
	{
		curr_node=curr_node->link;
		printf(" %d ",curr_node->data);
	}while(curr_node->link!=NULL);		//do-while end
}
			//outlink generation//
/**************
fun 	: finds the node to be pointed
param   : var-data node to be pointed
return	: req nodes address
***************/

struct node *find_node_addr(int var)
{
	struct node *present_node;
	present_node=root;
	do
	{
		if(present_node->data==var)
		{
		break;
		}
		present_node=present_node->link;
	}while(present_node->link!=NULL);
	return present_node;
}

/**************
fun 	: create a new node
param   : nil
return	: address of new_node
***************/

struct out_links *create_outlink_node(int temp_node_content)
{
	struct out_links *temp;
	temp=(struct out_links *)malloc(sizeof(struct out_links));
	temp->points_to=find_node_addr(temp_node_content);
	printf("(%d,%d)",temp->points_to->data,temp_node_content);
	temp->next_node=NULL;
	return temp;

}

/**************
fun 	: checks for duplicate copy of outlinks('var')
param   : total no of nodes,var-newly generated outlink,arrays starting address.ending address
return	: 'var'
***************/

int check(int total_node,int var,int *start_of_array,int *end_of_array)
{
	int *temp_array;
	temp_array=start_of_array;
	while(temp_array!=end_of_array)
	{
		if(*temp_array==var)
		{
			var=random(total_node)+1;
			temp_array=start_of_array;
			continue;
		}
		temp_array++;
	}
	return var;
}

/**************
fun 	: generates d outlinks for node passed
param   : total no of nodes,node for which outlinks needed to be generated
return	: NIL
***************/

void outlink_gen(int total_node,struct node *temp_curr_node)
{
	struct out_links *outlink_header,*curr_outlink_node,*next_outlink_node;
	int i,no_of_outlinks,node_content,temp_var1,*outlinks_array,*temp_array;	//node_content : has data of the node to be pointed ; outlinks_array : has values of nodes to be pointed & temp_array is used for manipulation of outlinks_array
	no_of_outlinks=random(total_node);
	printf("\nno of outlinks for node %d : %d",temp_curr_node->data,no_of_outlinks);
	temp_var1=no_of_outlinks;
	outlinks_array=(int *)malloc(sizeof(int)*no_of_outlinks);
	temp_array=outlinks_array;
	while(temp_var1!=0)	     // populate the array with non-duplicates
	{
		node_content=random(total_node)+1;
		*outlinks_array=check(total_node , node_content , temp_array , outlinks_array);
		printf("\npoint to (after dup elimination) : %d",*outlinks_array);
		outlinks_array++;
		temp_var1--;
	}
	outlinks_array=temp_array;
	temp_var1=no_of_outlinks;
	if(temp_var1!=0)
	{
		outlink_header=create_outlink_node(*outlinks_array);
		curr_outlink_node=outlink_header;
		temp_var1--;
	}
	else
	  outlink_header=NULL;  //the node has no outlink, outlink is assinged to NULL
	outlinks_array++;
	while(temp_var1!=0)	//gen link list of outlinks
	{
		node_content=*outlinks_array;
		next_outlink_node=create_outlink_node(node_content);
		curr_outlink_node->next_node=next_outlink_node;
		curr_outlink_node=next_outlink_node;
		temp_var1--;
		outlinks_array++;
	}
	temp_curr_node->list_of_outgoinglinks=outlink_header;
}

int main()
{
	int graph_size,temp_size;
	FILE *ptr;
	struct node *traversal_node;
	clrscr();
	setbuf(stdout,(char*)0);                //flushing output buffer
	printf("\nEnter the size of graph :");
	scanf("%d",&graph_size);
	graph_gen(graph_size);
	traversal_node=root;
	while(traversal_node->link!=NULL)
	{
		outlink_gen(graph_size,traversal_node);
		traversal_node=traversal_node->link;
	}
	outlink_gen(graph_size,traversal_node);
	traversal_node=traversal_node->link;

	traversal_node=root;
	curr_node=root;
	temp_size=graph_size;

	ptr=fopen("input.txt","w");          //open file for store the node
	while(temp_size!=0)
	{
	  fprintf(ptr,"%d",curr_node->data);
	  curr_node=curr_node->link;
	  outlink=traversal_node->list_of_outgoinglinks;
	    while(outlink->next_node!=NULL)
	    {
	      traversal_node=outlink->points_to;
	      fprintf(ptr," %d",traversal_node->data);
	      outlink=outlink->next_node;
	    }                                 //while end
	 traversal_node=outlink->points_to;
	    if(traversal_node->data!=0)
	       fprintf(ptr," %d",traversal_node->data);
	 traversal_node=curr_node;
	    if(temp_size!=1)
	       fprintf(ptr,"\n");
	 temp_size--;
	}                                  //while end
	fclose(ptr);                       //close file
	getch();
	return 0;

}