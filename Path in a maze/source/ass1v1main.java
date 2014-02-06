/**
 * 
 */

import java.io.BufferedReader;
//import java.io.FileNotFoundException;
import java.io.FileReader;
//import java.io.IOException;
import java.lang.System;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;
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

			find_root(cell,height,width);
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
			System.out.println("Q count: "+qcnt);
		}
		catch (Exception ex)
		{
			ex.printStackTrace();
		}
	}	

	public static void find_root(Grid cell[][],int height,int width)
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

	public static List<Grid> expand_to_find_children(Grid current,int height,int width,Grid cell[][],Queue<Grid> q)
	{
		List<Grid> child=new ArrayList<Grid>(4);
		int x,y;

		x=current.get_current_posx();
		y=current.get_current_posy();
		if(x+1 < width)
			if((cell[y][x+1].get_parent()==null)&&(!cell[y][x+1].equals(Grid.root)))
			{
				child.add(cell[y][x+1]);
				cell[y][x+1].set_parent(current);
				//q.add(cell[y][x +1]);
			}
		if(y+1 < height)
			if((cell[y+1][x].get_parent()==null)&&(!cell[y+1][x].equals(Grid.root)))
			{
				child.add(cell[y+1][x]);
				cell[y+1][x].set_parent(current);
				//q.add(cell[y+1][x]);
			}
		if(x-1 >= 0)
			if((cell[y][x-1].get_parent()==null)&&(!cell[y][x-1].equals(Grid.root)))
			{
				child.add(cell[y][x-1]);
				cell[y][x-1].set_parent(current);
				//q.add(cell[y][x-1]);
			}
		if(y-1 >= 0)
			if((cell[y-1][x].get_parent()==null)&&(!cell[y-1][x].equals(Grid.root)))
			{
				child.add(cell[y-1][x]);
				cell[y-1][x].set_parent(current);
				//q.add(cell[y-1][x]);
			}
		current.add_child(child);
		return child;
	}
}