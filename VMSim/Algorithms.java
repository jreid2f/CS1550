import java.io.*;
import java.util.*;

/*
 * CS 1550
 * Project 3
 * @author Joseph Reidell
 */

public class Algorithms {

	public Scanner scan; // Global Scanner method
	public File f; // Global File method
	public Testing[] table; // Instantiation of the page table
	public Testing[] RAM; // Instantiation of the RAM page table
	public int access = 0; // Used to keep track of the amount of memory accesses
	public int frames; // Used to keep track of the amount of frames entered
	public int faults = 0; // Used to keep track of the amount of page faults
	public int write = 0; // Used to keep track of the amount of writes to the disk
	public int frameRate = 0; // Used to keep track of the amount of active frame rates
	private final int SIZE = (int) Math.pow(2, 12); // Used to keep check the size of the page table

	// Constructor method
	public Algorithms(int num) {
		frames = num;
	}

	/*
	 * Method makes a page table for RAM to be used and sets the amount of frames
	 * that can be used
	 */
	public Testing[] makeRAM() {
		RAM = new Testing[frames];
		for (int i = 0; i < RAM.length; i++) {
			Testing test = new Testing();
			test.setFrames(i);
			RAM[i] = test;
		}
		return RAM;
	}

	/*
	 * Method creates a page table of a certain size and sets the index for the
	 * table.
	 */
	public Testing[] makeTable() {
		table = new Testing[(int) Math.pow(2, 20)];
		for (int i = 0; i < table.length; i++) {
			Testing test = new Testing();
			test.setIndex(i);
			table[i] = test;
		}
		return table;
	}

	/*
	 * Method is used to dereference bits
	 */
	public Testing[] clearBits(Testing[] ref) {
		for (int i = 0; i < frames; i++) {
			ref[i].setReference(false);
		}
		return ref;
	}

	/*
	 * Opt algorithm: simulates what the optimal page replacement algorithm would
	 * choose if it had perfect knowledge. Close to impossible to implement.
	 */
	public void opt(String file) {
		ArrayList<LinkedList<Integer>> num;
		scan = new Scanner(System.in);
		f = new File(file);
		int iterate = 0;
		// int remove = -1;

		try {
			scan = new Scanner(f);
		} catch (Exception e) {
			System.out.println("Could not find file!");
			System.exit(0);
		}

		table = makeTable();
		RAM = makeRAM();
		num = new ArrayList<LinkedList<Integer>>(table.length);

		/*
		 * Used to load in the table and make sure the file doesn't get written out of
		 * bounds.
		 */
		for (int i = 0; i < table.length; i++) {
			num.add(i, new LinkedList<Integer>());
		}

		while (scan.hasNextLine()) {
			String input = scan.nextLine();
			String[] split = input.split(" ");
			long memAdd = Long.decode("0x" + split[0]);
			int page_size = (int) (memAdd / SIZE);

			if (page_size < 0 || page_size >= table.length) {
				System.out.println("Writing paper out of bounds!");
				System.exit(0);
			}

			num.get(page_size).add(iterate);
			iterate++;
		}

		/*
		 * In order to iterate through the file from the beginning it has to be scanned
		 * in again. Easier to just add a new iteration of scan.
		 */
		try {
			scan = new Scanner(f);
		} catch (Exception e) {
			System.out.println("Could not find file!");
			System.exit(0);
		}

		/*
		 * Scan through each line and split at the space.
		 */
		while (scan.hasNextLine()) {
			access++;
			String input = scan.nextLine();
			String[] split = input.split(" ");
			char line = split[1].charAt(0);
			long memAdd = Long.decode("0x" + split[0]);
			int page_size = (int) (memAdd / SIZE);

			if (page_size < 0 || page_size >= table.length) {
				System.out.println("Writing paper out of bounds!");
				System.exit(0);
			}

			if (table[page_size].getValidity()) {
				if (line == 'W') {
					RAM[table[page_size].getFrames()].setDirt(true);
				}
				System.out.println("hit");
			}

			else {
				faults++;
				if (frameRate < frames) {
					table[page_size].setValidity(true);
					table[page_size].setFrames(frameRate);

					if (line == 'W') {
						table[page_size].setDirt(true);
					}
					RAM[frameRate] = table[page_size];
					System.out.println("page fault - no eviction");
					frameRate++;
				} else {
					int remove = -1;
					for (int i = 0; i < frames; i++) {
						if (num.get(RAM[i].getIndex()).peek() == null) {
							remove = i;
							break;
						} else {
							if (remove == -1) {
								remove = i;
							} else {
								if ((int) num.get(RAM[remove].getIndex()).peek() < (int) num.get(RAM[i].getIndex())
										.peek()) {
									remove = i;
								}
							}
						}
					}

					if (RAM[remove].getDirt()) {
						write++;
						System.out.println("page fault - evict dirty");
					} else {
						System.out.println("page fault - evict clean");
					}

					int temp = RAM[remove].getIndex();
					table[temp].setDirt(false);
					table[temp].setFrames(-1);
					table[temp].setValidity(false);
					RAM[remove] = table[page_size];

					if (line == 'W') {
						RAM[remove].setDirt(true);
					}
					RAM[remove].setValidity(true);
					RAM[remove].setReference(true);
					RAM[remove].setFrames(remove);
				}
			}

			num.get(page_size).remove();
		}

		summary("OPT", access, faults, write);

		access = 0;
		faults = 0;
		write = 0;
		frameRate = 0;

	}

	/*
	 * Clock algorithm: Uses the better implementation of the second-chance
	 * algorithm.
	 */
	public void clock(String file) {
		scan = new Scanner(System.in);
		f = new File(file);

		try {
			scan = new Scanner(f);
		} catch (Exception e) {
			System.out.println("Could not find file!");
			System.exit(0);
		}

		table = makeTable();
		RAM = makeRAM();

		while (scan.hasNextLine()) {
			access++;
			String input = scan.nextLine();
			String[] split = input.split(" ");
			char line = split[1].charAt(0);
			long memAdd = Long.decode("0x" + split[0]);
			int page_size = (int) (memAdd / SIZE);

			if (page_size < 0 || page_size >= table.length) {
				System.out.println("Writing paper out of bounds!");
				System.exit(0);
			}

			if (table[page_size].getValidity()) {
				if (line == 'W') {
					RAM[table[page_size].getFrames()].setDirt(true);
				}
				RAM[table[page_size].getFrames()].setReference(true);
				System.out.println("hit");
			}

			else {
				faults++;
				if (RAM[frameRate].getIndex() == -1) {
					table[page_size].setValidity(true);
					table[page_size].setFrames(frameRate);
					if (line == 'W')
						table[page_size].setDirt(true);
					table[page_size].setReference(true);
					RAM[frameRate] = table[page_size];
					System.out.println("page fault - no eviction");
					frameRate = (frameRate + 1) % frames;
				} else {
					for (;;) {
						if (!RAM[frameRate].getReference())
							break;
						else {
							RAM[frameRate].setReference(false);
						}
						frameRate = (frameRate + 1) % frames;
					}

					if (RAM[frameRate].getDirt()) {
						write++;
						System.out.println("page fault - evict dirty");
					} else
						System.out.println("page fault - evict clean");

					int temp = RAM[frameRate].getIndex();
					table[temp].setDirt(false);
					table[temp].setFrames(-1);
					table[temp].setValidity(false);
					RAM[frameRate] = table[page_size];
					if (line == 'W')
						RAM[frameRate].setDirt(true);
					RAM[frameRate].setValidity(true);
					RAM[frameRate].setReference(true);
					RAM[frameRate].setFrames(frameRate);
					frameRate = (frameRate + 1) % frames;
				}
			}

		}

		summary("Clock", access, faults, write);

		access = 0;
		faults = 0;
		write = 0;
		frameRate = 0;

	}

	/*
	 * Fifo algorithm: Implement first-in, first-out
	 */
	public void fifo(String file) {
		scan = new Scanner(System.in);
		f = new File(file);

		try {
			scan = new Scanner(f);
		} catch (Exception e) {
			System.out.println("Could not find file!");
			System.exit(0);
		}

		table = makeTable();
		RAM = makeRAM();

		while (scan.hasNextLine()) {
			access++;
			String input = scan.nextLine();
			String[] split = input.split(" ");
			char line = split[1].charAt(0);
			long memAdd = Long.decode("0x" + split[0]);
			int page_size = (int) (memAdd / SIZE);

			if (page_size < 0 || page_size >= table.length) {
				System.out.println("Writing paper out of bounds!");
				System.exit(0);
			}

			if (table[page_size].getValidity()) {
				if (line == 'W') {
					RAM[table[page_size].getFrames()].setDirt(true);
				}
				System.out.println("hit");
			}

			else {
				if (RAM[frameRate].getIndex() == -1) {
					table[page_size].setValidity(true);
					table[page_size].setFrames(frameRate);
					if (line == 'W')
						table[page_size].setDirt(true);
					RAM[frameRate] = table[page_size];
					System.out.println("page fault - no eviction");
				} else {
					if (RAM[frameRate].getDirt()) {
						write++;
						System.out.println("page fault - evict dirty");
					} else
						System.out.println("page fault - evict clean");

					int temp = RAM[frameRate].getIndex();
					table[temp].setDirt(false);
					table[temp].setFrames(-1);
					table[temp].setValidity(false);
					RAM[frameRate] = table[page_size];
					if (line == 'W')
						RAM[frameRate].setDirt(true);
					RAM[frameRate].setValidity(true);
					RAM[frameRate].setReference(true);
					RAM[frameRate].setFrames(frameRate);
				}

				faults++;
				frameRate = (frameRate + 1) % frames;
			}

		}

		summary("FIFO", access, faults, write);

		access = 0;
		faults = 0;
		write = 0;
		frameRate = 0;

	}

	/*
	 * NRU algorithm: Pick a not recently used page using the R and D bits.
	 */
	public void nru(int rate, String file) {
		scan = new Scanner(System.in);
		f = new File(file);
		// boolean refresh_refer = true;
		// boolean refresh_dirt = true;

		try {
			scan = new Scanner(f);
		} catch (Exception e) {
			System.out.println("Could not find file!");
			System.exit(0);
		}

		table = makeTable();
		RAM = makeRAM();

		while (scan.hasNextLine()) {
			access++;
			if (access % rate == 0) {
				RAM = clearBits(RAM);
			}
			String input = scan.nextLine();
			String[] split = input.split(" ");
			char line = split[1].charAt(0);
			long memAdd = Long.decode("0x" + split[0]);
			int page_size = (int) (memAdd / SIZE);

			if (page_size < 0 || page_size >= table.length) {
				System.out.println("Writing paper out of bounds!");
				System.exit(0);
			}

			if (table[page_size].getValidity()) {
				if (line == 'W') {
					RAM[table[page_size].getFrames()].setDirt(true);
				}
				RAM[table[page_size].getFrames()].setReference(true);
				System.out.println("hit");
			}

			else {
				faults++;
				if (frameRate < frames) {
					table[page_size].setValidity(true);
					table[page_size].setFrames(frameRate);
					if (line == 'W')
						table[page_size].setDirt(true);
					table[page_size].setReference(true);
					RAM[frameRate] = table[page_size];
					System.out.println("page fault - no eviction");
					frameRate++;
				} else {
					boolean refresh_refer = true;
					boolean refresh_dirt = true;
					int remove = -1;

					for (int i = 0; i < frames; i++) {
						boolean dirty = RAM[i].getDirt();
						boolean ref = RAM[i].getReference();

						if (!dirty && !ref) {
							remove = i;
							break;
						} else if (!ref) {
							if (refresh_refer) {
								remove = i;
								refresh_refer = false;
								refresh_dirt = true;
							}
						} else if (!dirty) {
							if (refresh_refer && refresh_dirt) {
								remove = i;
								refresh_dirt = false;
							}
						} else {
							if (remove == -1)
								remove = i;
						}
					}
					if (RAM[remove].getDirt()) {
						write++;
						System.out.println("page fault - evict dirty");
					} else
						System.out.println("page fault - evict clean");

					int temp = RAM[remove].getIndex();
					table[temp].setDirt(false);
					table[temp].setFrames(-1);
					table[temp].setValidity(false);
					RAM[remove] = table[page_size];
					if (line == 'W')
						RAM[remove].setDirt(true);
					RAM[remove].setValidity(true);
					RAM[remove].setReference(true);
					RAM[remove].setFrames(remove);
				}
			}

		}

		summary("NRU", access, faults, write);

		access = 0;
		faults = 0;
		write = 0;
		frameRate = 0;

	}

	public void summary(String algor, int memAccess, int fault, int writes) {
		System.out.println("Algorithm: " + algor);
		System.out.println("Number of Frames: " + frames);
		System.out.println("Total memory accesses: " + memAccess);
		System.out.println("Total page faults: " + fault);
		System.out.println("Total writes to disk: " + writes);
	}
}
