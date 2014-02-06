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
	private float speed;
	private float time;
	private int x;
	private int y;
	private char state;
	private List<Grid> adj_nodes=new ArrayList<Grid>(4);
	//private Grid parent;
	private boolean flag_visited;
	
	public Grid(int i,int j,char state) 
	{
		this.x=i;
		this.y=j;
		this.state=state;
		this.speed=0;
		this.time=0;
		//this.parent=null;
		this.flag_visited=false;
	}
		
	public void display()
	{
		System.out.println("Node:("+this.y+","+this.x+")");
		System.out.println("Speed: "+this.speed);
		System.out.println("Time: "+this.time);
		System.out.println("state: "+this.state);
		//System.out.println("No.of children: "+this.adj_nodes.size());
		if (adj_nodes.size()!=0)
		{
			for (Grid g:adj_nodes)
				System.out.println("child: ("+g.get_current_posy()+","+g.get_current_posx()+")");
		}
		/*if (this.parent!=null)
			System.out.println("Parent: ("+this.parent.get_current_posy()+","+this.parent.get_current_posx()+")");
		else
			System.out.println("No parent");*/
		return;
	}
	
	public char get_state()
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
	
	public List<Grid> get_adj_nodes()
	{
		return this.adj_nodes;
	}
	
	public void set_visited()
	{
		this.flag_visited=true;
		return;
	}
	
	public void set_speed(Float s)
	{
		this.speed=s;
		return;
	}
	
	public void set_time(Float t)
	{
		this.time=t;
		return;
	}
	
	public void add_adj_nodes(Grid adj_node)
	{
		this.adj_nodes.add(adj_node);
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

	public boolean get_visited_flag() 
	{
		return this.flag_visited;
	}
	
	/*public void set_parent(Grid pred)
	{
		this.parent=pred;
	}
	public Grid get_parent()
	{
		return this.parent;
	}*/
}
