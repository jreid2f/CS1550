/*
 * CS 1550
 * Project 3
 * @author Joseph Reidell
 */

public class vmsim {
	/*
	 * ./vmsim –n <numframes> -a <opt|clock|fifo|nru> [-r <refresh>] <tracefile>
	 * 
	 * -n = args[0] (need to check for '-n' in command line)
	 * <numframes> = args[1] (need to check for an integer)
	 * -a = args[2] (need to check for '-a' in command line)
	 * <opt|clock|fifo|nru> = args[3] (need to check for correct algorithm entered)
	 * [-r <refresh>] = args[4] (need to check for an integer, refresh rate, only nru)
	 * <tracefile> = args[5] (need to check for correct file is entered)
	 * 
	 * Overall need to check for at least 6 command line arguments
	 * 
	 * 
	 */
	public static void main(String[] args) {
		String algorithm;
		int f = 0;
		int rate = 0;
		
		if(args.length < 5 || args.length > 7) {
			System.out.println("Not enough arguments entered!");
			System.exit(0);
		}
		
		if(!args[0].equals("-n")) {
			System.out.println("First argument is invalid!");
			System.exit(0);
		}
		
		try {
			f = Integer.parseInt(args[1]);
		}
		catch(Exception e) {
			System.out.println("Second argument is invalid! Must be an integer!");
			System.exit(0);
		}
		
		if(!args[2].equals("-a")) {
			System.out.println("Third argument is invalid!");
			System.exit(0);
		}
		
		algorithm = args[3];
		if(!algorithm.equals("opt") &&  !algorithm.equals("clock") && 
				!algorithm.equals("fifo") && !algorithm.equals("nru")) {
			System.out.println("Fourth argument is invalid! Invalid algorithm entered!");
			System.exit(0);
		}
		
		Algorithms sim = new Algorithms(f);
		
		if(algorithm.equals("nru")) {
			if(args.length != 7) {
				System.out.println("Not enough arguments entered!");
				System.exit(0);
			}
			
			if(!args[4].equals("-r")) {
				System.out.println("Fifth argument is invalid!");
				System.exit(0);
			}
			
			try {
				rate = Integer.parseInt(args[5]);
			}
			catch(Exception e) {
				System.out.println("Sixth argument is invalid! Must be an integer!");
				System.exit(0);
			}
		}
		
		else if(args.length != 5) {
			System.out.println("Not enough arguments! entered");
			System.exit(0);
		}
		
		if(algorithm.equals("opt")) {
			sim.opt(args[4]);
		}
		else if(algorithm.equals("clock")) {
			sim.clock(args[4]);
		}
		else if(algorithm.equals("fifo")) {
			sim.fifo(args[4]);
		}
		else if(algorithm.equals("nru")) {
			sim.nru(rate, args[6]);
		}
		else {
			System.out.println("Invalid algorithm entered!");
			System.exit(0);
		}
	}
}
