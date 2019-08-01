
/*
 * CS 1550
 * Project 3
 * @author Joseph Reidell
 */

public class Testing {
	private int frames;
	private boolean validity;
	private boolean dirty;
	private boolean reference;
	private int index;
	
	public Testing() {
		frames = 0;
		validity = false;
		dirty = false;
		reference = false;
		index = 0;
	}
	
	public void setIndex(int point) {
		index = point;
	}
	
	public int getIndex() {
		return index;
	}
	
	public void setValidity(boolean valid) {
		validity = valid;
	}
	
	public boolean getValidity() {
		return validity;
	}
	
	public void setDirt(boolean dirt) {
		dirty = dirt;
	}
	
	public boolean getDirt() {
		return dirty;
	}
	
	public void setReference(boolean ref) {
		reference = ref;
	}
	
	public boolean getReference() {
		return reference;
	}
	
	public void setFrames(int frame) {
		frames = frame;
	}
	
	public int getFrames() {
		return frames;
	}
}
