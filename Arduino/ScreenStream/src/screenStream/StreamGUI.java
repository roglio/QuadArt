// Fast Serial Screen Streaming, written by Markus Lipp, based on Examples from AdaFruit
// BSD license, all text above must be included in any redistribution.

package screenStream;

import java.awt.Color;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.GridLayout;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.Robot;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentEvent;
import java.awt.event.ComponentListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.geom.AffineTransform;
import java.awt.image.AffineTransformOp;
import java.awt.image.BufferedImage;
import java.util.Timer;
import java.util.TimerTask;

import javax.swing.BoxLayout;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextField;

import jssc.SerialPort;
import jssc.SerialPortEvent;
import jssc.SerialPortEventListener;
import jssc.SerialPortException;
import jssc.SerialPortList;


public class StreamGUI {

	static int width = 100;
	static int height = 100;
	static int offsetX = 100;
	static int offsetY = 100;
	
	static JPanel contentPane;
	
	static private Robot robot;
	static BufferedImage after=null;
	
	static JPanel imgView = null;
	
	static int outputSizeX = 32;
	static int outputSizeY = 32;
	
	static SerialPort serialPort = null;

	static JLabel lFPS;
	
	static JComboBox<String> combo2=null;
			
	interface IChangeValue
	{
		void changeValue(int v);
	}	

	private static class ActionListenerImpl implements ActionListener
	{
		JTextField tf;
		IChangeValue fct;
		
		public ActionListenerImpl(JTextField _tf,IChangeValue _fct)
		{
			tf = _tf;
			fct = _fct;
		}	
		
		private void checkValue()
		{
			try
			{
				int v = new Integer(tf.getText());
				fct.changeValue(v);
			}
			catch (NumberFormatException e)
			{				
			}
			
		}

		@Override
		public void actionPerformed(ActionEvent e) {
			checkValue();
			
		}
	}
	
	public static void main(String[] args) {	
		//testDrawPixels();
		
		JFrame.setDefaultLookAndFeelDecorated(true);
		
		try {
			robot = new Robot();

			final JFrame frame = new JFrame();
			frame.setName("PixelController ScreenCapture Area");

			frame.setAlwaysOnTop(true); 
        
			frame.setUndecorated(true);
			frame.setBackground(new Color(0,0,0,0));
			frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
						
			contentPane = new JPanel() {	        	 
				private static final long serialVersionUID = 1L;
				private Color bg = new Color(240, 240, 240, 0);
	 
	            @Override
	            protected void paintComponent(final Graphics g) {
	                g.setColor(bg);
	                g.fillRect(0, 0, getWidth(), getHeight());
	            }
	        };
	        
	        frame.add(contentPane);
	        

			
			JFrame frame2 = new JFrame();
			Container pane2 = frame2.getContentPane();
			pane2.setLayout(new BoxLayout(pane2, BoxLayout.Y_AXIS));
			

			
			
			frame2.setSize(320, 620);			
			imgView = new JPanel() {	        	 
				private static final long serialVersionUID = 1L;
				private Color bg = new Color(240, 240, 240, 255);
				
	            @Override
	            protected void paintComponent(final Graphics g) {
	            	int w = getWidth();
	            	int h = getHeight();
	            	
	                g.setColor(bg);
	                g.fillRect(0, 0, getWidth(), getHeight());
	                
	                
	                /* for debugging interleaved buffer:
	                Graphics2D g2d = (Graphics2D)g;
	                if (matrixbuff!=null && matrixbuff.length==4096)
	                for (int x=0; x<32; x++)
	                	for (int y=0; y<32;y++)
	                	{
	                		int[] col = getPixelColor888(x, y, matrixbuff);
	                		g2d.setColor(new Color(col[0],col[1],col[2]));
	                		g2d.drawLine(x, y, x, y);
	                	}*/
	            	
	                if (after!=null)
	            		g.drawImage(after, 0, 0,  w,h, 0,0,outputSizeX,outputSizeY,null);
	            	
	            	float xStep = (float)w/(float)outputSizeX;
	            	float yStep = (float)h/(float)outputSizeY;
	            	
	            	for (int i=1; i<outputSizeX; i++)
	            		g.drawLine((int)((float)i*xStep), 0, (int)((float)i*xStep), h);
	            	
	            	for (int i=1; i<outputSizeY; i++)
	            		g.drawLine(0, (int)((float)i*yStep), w, (int)((float)i*yStep));
	            }
	        };
	        imgView.setPreferredSize(new Dimension(320,320));
	        pane2.add(imgView);

	        
	        
	        JLabel l1 = new JLabel("Capture Width");
	        JLabel l2 = new JLabel("Capture Height");
	        
	        JLabel l3 = new JLabel("Output Width");
	        JLabel l4 = new JLabel("Output Height");
	        
	        JLabel l5 = new JLabel("COM Port");
	        JLabel l6 = new JLabel("BAUD");
	        final JLabel l9 = new JLabel("COM Received: ");
	        
        
	        lFPS = new JLabel("FPS: ");
	        
	        final JCheckBox c1 = new JCheckBox("Enable Output");
	        
	        final JComboBox<?> combo = new JComboBox<Object>(getComPorts());
	        combo.setMaximumSize( combo.getPreferredSize());
	        
	        JLabel l8 = new JLabel("Output mode:");
	        
	        combo2 = new JComboBox<String>();
	        combo2.addItem("RGB4 buffer (Arduino)");
	        combo2.addItem("Full RGB8 (Teensy MatrixLib)");
	        combo2.addItem("Interleaved RGB8 (Teensy Simple Stream)");
	        combo2.setSelectedIndex(2);
	        combo2.setMaximumSize( combo.getPreferredSize());
	        
	        final JTextField tfWidth = new JTextField(10);
	        tfWidth.setMaximumSize( tfWidth.getPreferredSize());
	        final JTextField tfHeight = new JTextField(10);
	        tfHeight.setMaximumSize( tfHeight.getPreferredSize());
	        
	        JTextField tfOWidth = new JTextField(10);
	        tfOWidth.setText(""+outputSizeX);
	        tfOWidth.setMaximumSize( tfOWidth.getPreferredSize());
	        JTextField tfOHeight = new JTextField(10);
	        tfOHeight.setMaximumSize( tfOHeight.getPreferredSize());
	        tfOHeight.setText(""+outputSizeY);
	        
	        final JTextField tfBaud = new JTextField(10);
	        tfBaud.setText("250000");
	        tfBaud.setMaximumSize( tfBaud.getPreferredSize());
	        
			JPanel subPanel = new JPanel(new GridLayout(0,2));
			
			subPanel.add(l1); subPanel.add(tfWidth);
			subPanel.add(l2); subPanel.add(tfHeight);
			subPanel.add(l3); subPanel.add(tfOWidth);
			subPanel.add(l4); subPanel.add(tfOHeight);
	        
			subPanel.add(l5); subPanel.add(combo); 
			subPanel.add(l6); subPanel.add(tfBaud);
			subPanel.add(c1);
			subPanel.add(lFPS);

			subPanel.add(l8);
			subPanel.add(combo2);
			subPanel.add(l9);

			pane2.add(subPanel);
	        
			frame2.setVisible(true);
			frame2.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
			
			tfWidth.addActionListener(new ActionListenerImpl(tfWidth, new IChangeValue(){
				@Override
				public void changeValue(int v) {
					frame.remove(contentPane);
					contentPane.setPreferredSize(new Dimension(v, contentPane.getHeight()));
					frame.add(contentPane);
					frame.pack();
				}}
			));
			
			tfHeight.addActionListener(new ActionListenerImpl(tfHeight, new IChangeValue(){
				@Override
				public void changeValue(int v) {
					frame.remove(contentPane);
					contentPane.setPreferredSize(new Dimension(contentPane.getWidth(), v));
					frame.add(contentPane);
					frame.pack();
				}}
			));
			
			tfOWidth.addActionListener(new ActionListenerImpl(tfOWidth, new IChangeValue(){
				@Override
				public void changeValue(int v) {
					outputSizeX = v;
				}}
			));
			tfOHeight.addActionListener(new ActionListenerImpl(tfOHeight, new IChangeValue(){
				@Override
				public void changeValue(int v) {
					outputSizeY = v;
				}}
			));
			
			combo.addItemListener(new ItemListener() {
				
				@Override
				public void itemStateChanged(ItemEvent e) {
				    if (e.getStateChange() == ItemEvent.SELECTED) {
				    } else {
				    }					
				}
			});
			
	        frame.addWindowListener( new WindowAdapter() {

	        	@Override
	            public void windowClosing(WindowEvent e) {
	                int confirm = JOptionPane.showOptionDialog(frame,
	                        "Are You Sure to Close this Application?",
	                        "Exit Confirmation", JOptionPane.YES_NO_OPTION,
	                        JOptionPane.QUESTION_MESSAGE, null, null, null);
	                if (confirm == JOptionPane.YES_OPTION) {
	                    System.exit(0);
	                }
	            }
	        });
	        
			class SerialPortReader implements SerialPortEventListener {

				public void serialEvent(SerialPortEvent event) {
					if (event.isRXCHAR() && event.getEventValue() > 0) {
						int bytesCount = event.getEventValue();
						try {
							l9.setText("COM Recieved: "
									+ serialPort.readString(bytesCount));
						} catch (SerialPortException e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
					}
				}
			}
	        
	        c1.addItemListener(new ItemListener() {
	            public void itemStateChanged(ItemEvent e) {

	            	combo.setEnabled(!c1.isSelected());
	            	tfBaud.setEnabled(!c1.isSelected());
	            	if (!c1.isSelected())
	            	{
	            		if (serialPort!=null)
	            		{
	            			try {
	            				if (serialPort.isOpened())
	            					serialPort.closePort();
							} catch (SerialPortException e1) {								
								JOptionPane.showMessageDialog(null, e1.toString(), "Error closing COM", JOptionPane.OK_OPTION);
							}
	            			serialPort = null;	            			
	            		}
	            			
	            	}
	            	else
	            	{	
	            		try {
	            			Integer baud = new Integer(tfBaud.getText());
	            			serialPort = new SerialPort((String)combo.getSelectedItem());
	            			serialPort.openPort();
	            			serialPort.setParams(baud, 8, 1, 0);
	            			serialPort.addEventListener(new SerialPortReader());
	            			///serialPort.addEventListener(new Reader());
	                    }
	            		catch (NumberFormatException ex)
	            		{
	            			JOptionPane.showMessageDialog(null, "Illegal Baud Rate", "Illegal Baud Rate", JOptionPane.OK_OPTION);
	            		}
	                    catch (SerialPortException ex){
	                    	JOptionPane.showMessageDialog(null, ex.toString(), "Error opening COM", JOptionPane.OK_OPTION);
	                    	if (serialPort!=null && serialPort.isOpened())
								try {
									serialPort.closePort();
								} catch (SerialPortException e1) {
									// TODO Auto-generated catch block
									JOptionPane.showMessageDialog(null, e1.toString(), "Error closing COM", JOptionPane.OK_OPTION);
								}
	                        serialPort = null;
	                    }


	            		
	            	}
	            }
	        });
	        
			contentPane.addComponentListener(new ComponentListener(){

				@Override
				public void componentHidden(ComponentEvent arg0) {
					
					
				}

				@Override
				public void componentMoved(ComponentEvent arg0) {
					
					
				}

				@Override
				public void componentResized(ComponentEvent arg0) {
					
					tfWidth.setText(""+contentPane.getWidth());
					tfHeight.setText(""+contentPane.getHeight());
				}

				@Override
				public void componentShown(ComponentEvent arg0) {
					
					
				}});
			
			
			frame.setSize(width, height);
			frame.setLocation(offsetX,offsetY);  
			frame2.setLocation(offsetX*2,offsetY*2);  
			frame.setVisible(true);
		} catch (Exception e) {
			
		}
		
		Timer timer = new Timer();
		
		timer.scheduleAtFixedRate(new TimerTask() {
			  @Override
			  public void run() {
				  update();              	 
                      
				  
			  }
			}, 1, 1);
		


	}	
	
	public static String[] getComPorts()
	{
		return SerialPortList.getPortNames();
	}

	
	static int getTargetRow(int y)
	{
		return y%16;
	}
	
	static int getTargetCol(int x, int bit)
	{
		return x+bit*32;
	}
	
	
	static boolean getTargetHigh(int y)
	{
		return y>=16;
	}
	 
	static void testDrawPixels()
	{
		assert(getTargetRow(0)==0);
		assert(getTargetRow(1)==1);
		assert(getTargetRow(7)==7);
		assert(getTargetRow(8)==8);
		assert(getTargetRow(15)==15);
		assert(getTargetRow(16)==0);
		assert(getTargetRow(31)==15);
		
		assert(getTargetHigh(0)==false);
		assert(getTargetHigh(1)==false);
		assert(getTargetHigh(7)==false);
		assert(getTargetHigh(8)==false);
		assert(getTargetHigh(15)==false);
		assert(getTargetHigh(16)==true);
		assert(getTargetHigh(31)==true);
		
		assert(getTargetCol(0,0)==0);		
		assert(getTargetCol(1,0)==1);
		assert(getTargetCol(31,0)==31);
		
		assert(getTargetCol(0,1)==32);
		assert(getTargetCol(31,1)==63);
		
		assert(getTargetCol(0,2)==64);
		
		byte target[] = new byte[32*32*4];
		
		for (int x=0; x<32;x++)
		{
			for (int y=0; y<32;y++)
			{
				drawPixel888(x,y,(byte)255,(byte)255,(byte)255, target);
			}
		}
		
		for (int i=0; i<target.length; i++)
		{
			assert(target[i]==63);
		}
		
		target = new byte[32*32*4];
		
		
		for (int x=0; x<32;x++)
		{
			for (int y=0; y<32;y++)
			{
				drawPixel888(x,y,(byte)0,(byte)0,(byte)0, target);
			}
		}
		
		for (int i=0; i<target.length; i++)
		{
			assert(target[i]==0);
		}
		
		
		target = new byte[32*32*4];
		
		for (int x=0; x<32;x++)
		{
			for (int y=0; y<32;y++)
			{
				drawPixel888(x,y,(byte)255,(byte)0,(byte)0, target);
			}
		}
		
		//should be 0000r00r
		for (int i=0; i<target.length; i++)
		{
			assert(  (target[i]&1)!=0 && (target[i]&8)!=0);
			assert(  (target[i]&2)==0 && (target[i]&4)==0);
			assert(  (target[i]&16)==0 && (target[i]&32)==0);
		}
		
		target = new byte[32*32*4];
		
		for (int y=0; y<32;y++)
		{
			for (int x=0; x<32;x++)
			{
				drawPixel888(x,y,(byte)170,(byte)0,(byte)0, target); 
			}
		}
		
	
		for (int i=0; i<target.length; i++)
		{
			int bam =(i%256)/32;
			
			if (bam%2==0)
				assert(target[i]==0);
			else
				assert( target[i]!=0);
		}
		
		
	}
	
	static int[] getPixelColor888(int x, int y, byte buffer[])
	{
		int rgb[] = new int[3];
		
		int rowLength = 32*8; 
		int targetRow =getTargetRow(y);	    
		boolean targetHigh =  getTargetHigh(y);
		int baseAddr = targetRow*rowLength;
		
		for (int i=0; i<8; i++)
		{
			int baseAddrCol = baseAddr+getTargetCol(x,i);
			int bit = 1<<i;			
			
			if ((buffer[baseAddrCol]&(targetHigh?8:1))!=0)
				rgb[0] |= bit;
			if ((buffer[baseAddrCol]&(targetHigh?16:2))!=0)
				rgb[1] |= bit;
			if ((buffer[baseAddrCol]&(targetHigh?32:4))!=0)
				rgb[2] |= bit;
			
		}
		
		return rgb;
	}
	
	static void drawPixel888(int x, int y, byte r, byte g, byte b, byte target[]) {		
		
		
		int rowLength = 32*8; 
				
		int targetRow =getTargetRow(y);	    
		boolean targetHigh =  getTargetHigh(y);
	    
		int baseAddr = targetRow*rowLength;
		for (int i=0; i<8; i++)
		{
			int baseAddrCol = baseAddr+getTargetCol(x,i);
			int bit = 1<<i;			
			
			target[baseAddrCol]&= targetHigh?7:56; //zero target bits
			
			if ((r & bit) != 0)
				target[baseAddrCol]|=targetHigh?8:1;
			if ((g & bit) != 0)
				target[baseAddrCol]|=targetHigh?16:2;
			if ((b & bit) != 0)
				target[baseAddrCol]|=targetHigh?32:4;
		}
		
	}
	
	//C To Java conversion from Adafruit Matrix Library
	static byte gamma[] = {
			  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			  0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,
			  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
			  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
			  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
			  0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x02,
			  0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
			  0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
			  0x02,0x02,0x02,0x02,0x02,0x03,0x03,0x03,
			  0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,
			  0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x04,
			  0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,
			  0x04,0x04,0x04,0x04,0x04,0x04,0x05,0x05,
			  0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,
			  0x05,0x05,0x05,0x06,0x06,0x06,0x06,0x06,
			  0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x07,
			  0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,
			  0x07,0x07,0x08,0x08,0x08,0x08,0x08,0x08,
			  0x08,0x08,0x08,0x08,0x09,0x09,0x09,0x09,
			  0x09,0x09,0x09,0x09,0x09,0x0a,0x0a,0x0a,
			  0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0b,0x0b,
			  0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0c,0x0c,
			  0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0d,0x0d,
			  0x0d,0x0d,0x0d,0x0d,0x0d,0x0e,0x0e,0x0e,
			  0x0e,0x0e,0x0e,0x0e,0x0f,0x0f,0x0f,0x0f
			};
	
	static int color888(
			  int r, int g, int b, boolean gflag) {
			  if(gflag) { // Gamma-corrected color?
			    r = gamma[r]; // Gamma correction table maps
			    g = gamma[g]; // 8-bit input to 4-bit output
			    b = gamma[b];
			    return (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
			           (g <<  7) | ((g & 0xC) << 3) |
			           (b <<  1) | ( b        >>> 3);
			  } // else linear (uncorrected) color
			  return ((r & 0xF8) << 11) | ((g & 0xFC) << 5) | (b >>> 3);
			}
	
	static byte matrixbuff[];
	static int nPlanes = 4;
	static int nRows = 16;
	static int WIDTH = 32;
	
	static void drawPixel(int x, int y, int c, byte[] arr) {
		byte r, g, b, bit, limit;
		
		// Adafruit_GFX uses 16-bit color in 5/6/5 format, while matrix needs
		// 4/4/4. Pluck out relevant bits while separating into R,G,B:
		r = (byte)((c >>> 12) & 0xF); // RRRRrggggggbbbbb
		g = (byte)((c >>> 7) & 0xF); // rrrrrGGGGggbbbbb
		b = (byte)((c >>> 1) & 0xF); // rrrrrggggggBBBBb

		//r=4;g=0;b=0;
		// Loop counter stuff
		bit = 2;
		limit = (byte)(1 << nPlanes);

		int baseAddr;

		if (y < nRows) {

			baseAddr = y * WIDTH * (nPlanes - 1) + x;// ptr =
														// &matrixbuff[backindex][y
														// * WIDTH * (nPlanes -
														// 1) + x]; // Base addr
			
			arr[baseAddr + 64] &= 252; // ptr[64] &= ~B00000011; // Plane 0 R,G
										// mask out in one op
			if ((r & 1) != 0)
				arr[baseAddr + 64] |= 1;  // if(r & 1) ptr[64] |= B00000001; //
											// Plane 0 R: 64 bytes ahead, bit 0
			if ((g & 1) != 0)
				arr[baseAddr + 64] |= 2;// if(g & 1) ptr[64] |= B00000010; //
										// Plane 0 G: 64 bytes ahead, bit 1
			if ((b & 1) != 0)
				arr[baseAddr + 32] |= 1;// if(b & 1) ptr[32] |= B00000001; //
										// Plane 0 B: 32 bytes ahead, bit 0
			else
				arr[baseAddr + 32] &= 254; // else ptr[32] &= ~B00000001; //
											// Plane 0 B unset; mask out

			for (; bit < limit; bit <<= 1) {// for(; bit < limit; bit <<= 1) {
				arr[baseAddr] &= 227; ; // *ptr &= ~B00011100; // Mask out R,G,B in
										// one op
				if ((r & bit) != 0)
					arr[baseAddr] |= 4; // if(r & bit) *ptr |= B00000100; //
										// Plane N R: bit 2
				if ((g & bit) != 0)
					arr[baseAddr] |= 8; // if(g & bit) *ptr |= B00001000; //
										// Plane N G: bit 3
				if ((b & bit) != 0)
					arr[baseAddr] |= 16; // if(b & bit) *ptr |= B00010000; //
											// Plane N B: bit 4
				baseAddr += WIDTH; // Advance to next bit plane
			}
		} else {

			baseAddr = (y - nRows) * WIDTH * (nPlanes - 1) + x; // ptr =
																// &matrixbuff[backindex][(y
																// - nRows) *
																// WIDTH *
																// (nPlanes - 1)
																// + x];
			arr[baseAddr] &=  252; // *ptr &= ~B00000011; // Plane 0 G,B mask out
								// in one op
			if ((r & 1) != 0)
				arr[baseAddr + 32] |= 2; // if(r & 1) ptr[32] |= B00000010; //
											// Plane 0 R: 32 bytes ahead, bit 1
			else
				arr[baseAddr + 32] &= 253; // else ptr[32] &= ~B00000010; //
											// Plane 0 R unset; mask out
			if ((g & 1) != 0)
				arr[baseAddr] |= 1;// if(g & 1) *ptr |= B00000001; // Plane 0 G:
									// bit 0
			if ((b & 1) != 0)
				arr[baseAddr] |= 2;// if(b & 1) *ptr |= B00000010; // Plane 0 B:
									// bit 0

			for (; bit < limit; bit <<= 1) {
				// for(; bit < limit; bit <<= 1) {
				arr[baseAddr] &= 31; // *ptr &= ~B11100000; // Mask out R,G,B
										// in one op
			if ((r & bit) != 0)
				arr[baseAddr] |= 32;// if(r & bit) *ptr |= B00100000; // Plane N
									// R: bit 5
			if ((g & bit) != 0)
				arr[baseAddr] |= 64;// if(g & bit) *ptr |= B01000000; // Plane N
									// G: bit 6
			if ((b & bit) != 0)
				arr[baseAddr] |= 128;// if(b & bit) *ptr |= B10000000; // Plane
										// N B: bit 7
			baseAddr += WIDTH; // Advance to next bit plane
			}
			
		}
	}

	static long prevTime = System.nanoTime();
	public static void update() {		
		   
		int outputSizeXcur = outputSizeX;
		int outputSizeYcur = outputSizeY;
		
		if (robot != null) {
			//get screenshot
			Rectangle rectangleCaptureArea = contentPane.getBounds();
			Point  s = contentPane.getLocationOnScreen(); 
			rectangleCaptureArea.x += s.x;
			rectangleCaptureArea.y += s.y;
			
			BufferedImage before = robot.createScreenCapture(rectangleCaptureArea);

			int w = Math.max(before.getWidth(),outputSizeXcur);
			int h = Math.max(before.getHeight(),outputSizeYcur);
			if (after == null || after.getWidth()!= w || after.getHeight()!=h)
				after = new BufferedImage(w, h, BufferedImage.TYPE_INT_ARGB);
			
			AffineTransform at = new AffineTransform();
			at.scale((float)outputSizeXcur/(float)w, (float)outputSizeYcur/(float)h);
			AffineTransformOp scaleOp = 
			   new AffineTransformOp(at, AffineTransformOp.TYPE_BILINEAR);
			after = scaleOp.filter(before, after);
		
		}		
		
		
		
		if (imgView!=null)
			  imgView.repaint();
		  
		 try {

			if (serialPort != null && serialPort.isOpened()) {
				


				if (combo2.getSelectedIndex() == 1) { // full rgb8
					
					//magic values			
					serialPort.writeByte((byte)(1));  
				
					matrixbuff = new byte[3 * outputSizeY * outputSizeX];

					int pIdx = 0;
					for (int y = 0; y < outputSizeY; y++) {
						for (int x = 0; x < outputSizeX; x++) {
							int argb = after.getRGB(x, y);

							Color c = new Color(argb);

							matrixbuff[pIdx * 3 + 0] = (byte) c.getRed();
							matrixbuff[pIdx * 3 + 1] = (byte) c.getGreen();
							matrixbuff[pIdx * 3 + 2] = (byte) c.getBlue();

							pIdx++;
						}
					}
					
					for (int i=0; i<matrixbuff.length; i++) //ensure magic numbers are unique
						  if (matrixbuff[i]==1)
							  matrixbuff[i]=(byte)2;
							
					
				} else if (combo2.getSelectedIndex() == 2) {
					
					//magic values			
					serialPort.writeByte((byte)(192)); //11000000 
					serialPort.writeByte((byte)(192)); 
					
					// interleaved rgb8
					matrixbuff = new byte[4 * outputSizeY * outputSizeX];

					int pIdx = 0;
					for (int y = 0; y < outputSizeY; y++) {
						for (int x = 0; x < outputSizeX; x++) {
							int argb = after.getRGB(x, y);
							Color c = new Color(argb);

							float ga = 2.2f;
							
							int r =c.getRed();
							int g =c.getGreen();
							int b =c.getBlue();				
							
							r = (byte)(Math.pow(((float)r)/255.0,ga)*255.0);
							g = (byte)(Math.pow(((float)g)/255.0,ga)*255.0);
							b = (byte)(Math.pow(((float)b)/255.0,ga)*255.0);
							
							drawPixel888(x,y,(byte)r,(byte)g,(byte)b,matrixbuff);
	
							pIdx++;
						}
					}
				} else if (combo2.getSelectedIndex() == 0) { // preprocessed

					//magic values			
					serialPort.writeByte((byte)(0x21)); //00001000 
					serialPort.writeByte((byte)(0x8)); //00100001
					
					matrixbuff = new byte[1536];

					for (int y = 0; y < outputSizeYcur; y++) {
						for (int x = 0; x < outputSizeXcur; x++) {
							int argb = after.getRGB(x, y);

							Color c = new Color(argb);

							int r = c.getRed();
							int g = c.getGreen();
							int b = c.getBlue();

							int c565 = color888(r, g, b, true);
							drawPixel(x, y, c565, matrixbuff);
						}

					}
					
					for (int i=0; i<matrixbuff.length; i++) //ensure magic numbers are unique
						  if (matrixbuff[i]==0x21) matrixbuff[i]=0;
							else if (matrixbuff[i]==0x8) matrixbuff[i]=0;
				}	

				
				if (serialPort==null)
					return;			
				serialPort.writeBytes(matrixbuff);
				
				/*DEBUG write, can be copied to sketch
				 * for (int i=0; i<matrixbuff.length; i++)
				{
					System.out.print(matrixbuff[i]+",");
					
					if (i%200==0)
						System.out.println("");
				}
				
				System.out.println("");
				System.out.println("");
				System.out.println("");*/
				
			}
		} catch (SerialPortException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} 
		
		long curTime = System.nanoTime();
		
		if (prevTime!=curTime && lFPS!=null)
		{
			lFPS.setText("FPS: "+1000000000/(curTime-prevTime));
		}
		
		prevTime = curTime;
	};


	
}
