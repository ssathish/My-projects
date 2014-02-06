/**
 * 
 */
import java.util.*;
/**
 * @author Sathish
 *
 */
public class Grid
{
	static Grid root;
	static float speed;
	static float time;
	private int x;
	private int y;
	private char state;
	private List<Grid> children=new ArrayList<Grid>(4);
	private Grid parent;
	//private boolean flag_expanded;
	
	public Grid(int i,int j,char state) 
	{
		this.x=i;
		this.y=j;
		this.state=state;
		this.parent=null;
		//this.flag_expanded=false;
	}
		
	public void display()
	{
		System.out.println("Node:("+this.y+","+this.x+")");
		//System.out.println("y pos: "+this.y);
		System.out.println("state: "+this.state);
		System.out.println("No.of children: "+this.children.size());
		if (children.size()!=0)
		{
			for (Grid g:children)
				System.out.println("child: ("+g.get_current_posy()+","+g.get_current_posx()+")");
		}
		if (this.parent!=null)
			System.out.println("Parent: ("+this.parent.get_current_posy()+","+this.parent.get_current_posx()+")");
		else
			System.out.println("No parent");
		return;
	}
	
	public char getstate()
	{
		return this.state;
	}
	
	public int get_current_posx()
	{
		return this.x;
	}
	
	public int get_current_posy()
	{
		return this.y;
	}
	
	public void set_parent(Grid pred)
	{
		this.parent=pred;
	}
	public Grid get_parent()
	{
		return this.parent;
	}
	public void add_child(List<Grid> child)
	{
		for (Grid tmp:child)
			this.children.add(tmp);
		return;
	}

	@Override
	public boolean equals(Object obj) 
	{
		Grid g=(Grid)obj;
		boolean isequal=false;
		if ((this.x==g.x)&&(this.y==g.y))
			isequal=true;
		return isequal;
	}
	
	
}
