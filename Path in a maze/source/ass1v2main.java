/**
 * 
 */

import java.io.BufferedReader;
//import java.io.FileNotFoundException;
import java.io.FileReader;
//import java.io.IOException;
import java.lang.System;
import java.util.List;
import java.util.ArrayList;
//import java.util.ArrayList;
//import java.util.LinkedList;
//import java.util.List;
//import java.util.Queue;
import java.util.Stack;
import java.util.StringTokenizer;

/**
 * @author Sathish
 *
 */

public class Main 
{
	public static void main(String args[]) 
	{
		try
		{
			String filename="C:\\Users\\Sathish\\Documents\\EclipseProjects\\JavaApp1\\src\\input1.txt";
			String line;
			String file_contents[]=new String[2];
			StringTokenizer st;
			int i=0,j=0,width=0,height=0;
			BufferedReader br=new BufferedReader(new FileReader(filename));

			for(i=0;i<args.length;i++)
			{
				System.out.println("arg "+i+" value: "+args[i]);
			}
			for(i=0;i<2;i++)
			{
				line=br.readLine();
				if(line!=null)
				{
					st=new StringTokenizer(line);
					j=0;
					while(st.hasMoreTokens())
					{
						file_contents[j]=st.nextToken();
						j++;
					}
					if(file_contents[0].equalsIgnoreCase("width"))
						width=Integer.parseInt(file_contents[1]);
					else if(file_contents[0].equalsIgnoreCase("height"))
						height=Integer.parseInt(file_contents[1]);
				}
			}
			System.out.println("width: " + width);
			System.out.println("height: " + height);

			Grid cell[][]=new Grid[height][width];
			for(i=0;i<height;i++)
			{
				line=br.readLine();
				for(j=0;j<width;j++)
				{
					cell[i][j]=new Grid(j,i,line.charAt(j));
				}
			}
			br.close();
			for(i=0;i<height;i++)
			{
				for(j=0;j<width;j++)
				{
					if(cell[i][j].get_state()=='S')
					{
						Grid.root=cell[i][j];
						cell[i][j].set_speed(Float.parseFloat(args[2]));
					}
					if(j+1 < width)
						cell[i][j].add_adj_nodes(cell[i][j+1]);
					if(i+1 < height)
						cell[i][j].add_adj_nodes(cell[i+1][j]);
					if(j-1 >= 0)
						cell[i][j].add_adj_nodes(cell[i][j-1]);
					if(i-1 >= 0)
						cell[i][j].add_adj_nodes(cell[i-1][j]);
				}
			}
			perform_dfs_to_find_path();
		}
		catch (Exception ex)
		{
			ex.printStackTrace();
		}
	}

	public static void perform_dfs_to_find_path()
	{
		boolean path_found=false;
		List<Grid> path=new ArrayList<Grid>();
		List<Grid> adj_nodes=new ArrayList<Grid>();
		Stack<Grid> s =new Stack<Grid>();
		Grid current;Grid neighbour=null;
		s.push(Grid.root);
		while(!s.isEmpty())
		{
			current=s.peek();
			chk_if_path_alread_added_and_remove(path,current);
			path.add(current);
			current.set_visited();
			if (current.get_state()=='G')
			{
				path_found=true;
				break;
			}

			adj_nodes=current.get_adj_nodes();
			for (Grid test:adj_nodes)
			{
				neighbour=test;
				if ((neighbour.get_state()!='*')&&(!neighbour.get_visited_flag()))
				{
					s.push(neighbour);
					break;
				}
				else
				{
					neighbour=null;
				}
			}
			if(neighbour==null)
			{
				if(!s.isEmpty())
				{
					//path.remove(current);
					current=s.pop();
				}
			}
		}
		if(path_found)
		{
			for (Grid tmp: path)
			{
				System.out.print("("+tmp.get_current_posy()+","+tmp.get_current_posx()+") ");
			}
		}
		else
		{
			System.out.println("Path not found using DFS");
			for (Grid tmp: path)
			{
				System.out.print("("+tmp.get_current_posy()+","+tmp.get_current_posx()+") ");
			}
		}
		return;
	}

	private static void chk_if_path_alread_added_and_remove(List<Grid> path,Grid current) 
	{
		int index,i;
		if(path.contains(current))
		{
			index=path.indexOf(current);
			for(i=index;i<=path.size();i++)
			{
				path.remove(index);
			}
		}
		return;
	}
}
/*current=Grid.root;
current.set_visited();
path.add(current);*/
/*if (!current.get_visited_flag())
{*/

//}
//System.out.println("("+current.get_current_posy()+","+current.get_current_posx()+") flag: "+current.get_visited_flag());
//System.out.println("hello"+neighbour.get_current_posy()+","+neighbour.get_current_posx()+" size: "+adj_nodes.size());
/*else
{
	if(!s.isEmpty())
	{
		current.set_visited();
		path.add(current);
		current=s.peek();
	}

}*/

/*find_root(cell,height,width);
	Queue<Grid> q=new LinkedList<Grid>();
	List<Grid> child=new ArrayList<Grid>(4);
	Grid current;
	q.add(Grid.root);
	int qcnt=0;

	while(!q.isEmpty())
	{
		current=q.remove();
		qcnt=qcnt+1;
		child=expand_to_find_children(current,height,width,cell,q);
		for (Grid tmp:child)
			q.add(tmp);
		current.display();
	}
	System.out.println("Q count: "+qcnt);*/


/*public static void find_root(Grid cell[][],int height,int width)
	{
		boolean found_root=false;

		for (int i=0;i<height;i++)
		{
			for(int j=0;j<width;j++)
			{
				if (cell[i][j].getstate()=='S')
				{
					Grid.root=cell[i][j];
					found_root=true;
					break;
				}
			}
			if(found_root)
				break;
		}
		return;
	}

	public static List<Grid> expand_to_find_adj_nodes(Grid current,int height,int width,Grid cell[][],Queue<Grid> q)
	{
		List<Grid> child=new ArrayList<Grid>(4);
		int x,y;

		x=current.get_current_posx();
		y=current.get_current_posy();

		return child;
	}*/

