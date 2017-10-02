
/*
 * Copyright (c) 2015, Nordic Semiconductor
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.active.maxq;




import java.io.UnsupportedEncodingException;
import java.text.DateFormat;
import java.util.Date;
import java.util.StringTokenizer;


import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import com.jjoe64.graphview.series.DataPoint;
import com.jjoe64.graphview.series.LineGraphSeries;


public class MainActivity extends Activity implements RadioGroup.OnCheckedChangeListener {
    private static final int REQUEST_SELECT_DEVICE = 1;
    private static final int REQUEST_ENABLE_BT = 2;
    private static final int UART_PROFILE_READY = 10;
    public static final String TAG = "MaxQ";
    private static final int UART_PROFILE_CONNECTED = 20;
    private static final int UART_PROFILE_DISCONNECTED = 21;
    private static final int STATE_OFF = 10;

    TextView mRemoteRssiVal;
    RadioGroup mRg;
    private int mState = UART_PROFILE_DISCONNECTED;
    private UartService mService = null;
    private BluetoothDevice mDevice = null;
    private BluetoothAdapter mBtAdapter = null;
    private ListView messageListView;
    private ArrayAdapter<String> listAdapter;
    private Button btnConnectDisconnect,btnlogger,btnfinalplot,buttonoff;
    //private EditText edtMessage;
    public static double[] stringId=new double[10000];
    public static double[] temperature=new double[10000];
    public static String[] PayloadStatus=new String[10000];
    public static String[] Box_Status=new String[10000];
    public static int[] Time=new int[1000];
    public  static String[] DateTime=new String[10000];
    public static String[] texttopdf = new String[10000];
    public static  int Icount=0;
    public static int j=0;
    public String[] string_array =new String[10000];
    public static String[] file_array =new String[10000];
    public static int string_i=0;
    public static int file_i=0;
    public static int string_count=1;
    public static int $_count=0;
    public static int hash_count=0;
    public String finaltext=null;

    LineGraphSeries<DataPoint> series1;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);
        mBtAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBtAdapter == null) {
            Toast.makeText(this, "Bluetooth is not available", Toast.LENGTH_LONG).show();
            finish();
            return;
        }
        messageListView = (ListView) findViewById(R.id.listMessage);
        listAdapter = new ArrayAdapter<String>(this, R.layout.message_detail);
        messageListView.setAdapter(listAdapter);
        messageListView.setDivider(null);
        btnConnectDisconnect=(Button) findViewById(R.id.btn_select);
        //btnSend=(Button) findViewById(R.id.sendButton);
        //edtMessage = (EditText) findViewById(R.id.sendText);
        //btnRealPlot=(Button)findViewById(R.id.real);

        //btnElapsedPlot=(Button)findViewById(R.id.elapsed);
        btnlogger=(Button)findViewById(R.id.btnlog);
        btnfinalplot=(Button)findViewById(R.id.btnfinal);
        //Button buttonoff = (Button) findViewById(R.id.btnoff);
        btnfinalplot.setEnabled(false);
        btnlogger.setEnabled(true);
        buttonoff=(Button)findViewById(R.id.btnoff);

        //GraphView graphview = (GraphView) findViewById(R.id.graph2);
        //LineGraphSeries<DataPoint> series1;
        series1 = new LineGraphSeries<DataPoint>();
        //graphview.addSeries(series1);
        //graphview.setTitle("Time vs Temperature graph");
        //GridLabelRenderer gridlabel = graphview.getGridLabelRenderer();
        //gridlabel.setHorizontalAxisTitle("Time in seconds");
        //gridlabel.setVerticalAxisTitle("Temperature in Celsius");
        //graphview.getViewport().setMinY(0);
        //graphview.getViewport().setMinX(0);
        //graphview.getViewport().setMaxY(35);
        //graph.getViewport().setMaxX(100);
        //graphview.getViewport().setScrollable(true);
        //graphview.getViewport().setYAxisBoundsManual(true);
        //graphview.getViewport().setXAxisBoundsManual(true);
        //graphview.getViewport().setScalable(true);
        //graphview.getViewport().setScalableY(true);
        //graphview.getViewport().setDrawBorder(true);
        series1.setDrawBackground(true);
        //series.setColor(Color.BLUE);

        service_init();

     
       
        // Handle Disconnect & Connect button
        btnConnectDisconnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!mBtAdapter.isEnabled()) {
                    Log.i(TAG, "onClick - BT not enabled yet");
                    Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                    startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
                }
                else {
                	if (btnConnectDisconnect.getText().equals("Connect")){
                		
                		//Connect button pressed, open DeviceListActivity class, with popup windows that scan for devices
                		
            			Intent newIntent = new Intent(MainActivity.this, DeviceListActivity.class);
            			startActivityForResult(newIntent, REQUEST_SELECT_DEVICE);
        			} else {
        				//Disconnect button pressed
        				if (mDevice!=null)
        				{
        					mService.disconnect();


        					
        				}
        			}
                }
            }
        });
        // Handle Send button
      /*  btnSend.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
            	//EditText editText = (EditText) findViewById(R.id.sendText);
            	//String message = editText.getText().toString();
            	byte[] value;
				try {
					//send data to service
					value = message.getBytes("UTF-8");
					mService.writeRXCharacteristic(value);
					//Update the log with time stamp

					String currentDateTimeString = DateFormat.getTimeInstance().format(new Date());
					listAdapter.add("["+currentDateTimeString+"] TX: "+ message);
               	 	messageListView.smoothScrollToPosition(listAdapter.getCount() - 1);
               	 	//edtMessage.setText("");
				} catch (UnsupportedEncodingException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
                
            }
        }); */
        btnlogger.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //EditText editText = (EditText) findViewById(R.id.sendText);
                //String message = editText.getText().toString();
                String message="on";
                byte[] value;
                try {
                    //send data to service
                    value = message.getBytes("UTF-8");
                    mService.writeRXCharacteristic(value);
                    //Update the log with time stamp

                    String currentDateTimeString = DateFormat.getTimeInstance().format(new Date());
                    listAdapter.add("["+currentDateTimeString+"] on");
                    messageListView.smoothScrollToPosition(listAdapter.getCount() - 1);
                    //edtMessage.setText("");
                } catch (UnsupportedEncodingException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }

            }
        });
        buttonoff.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //EditText editText = (EditText) findViewById(R.id.sendText);
                //String message = editText.getText().toString();
                String message="off";
                byte[] value;
                try {
                    //send data to service
                    value = message.getBytes("UTF-8");
                    mService.writeRXCharacteristic(value);
                    //Update the log with time stamp

                    String currentDateTimeString = DateFormat.getTimeInstance().format(new Date());
                    listAdapter.add("["+currentDateTimeString+"] off");
                    messageListView.smoothScrollToPosition(listAdapter.getCount() - 1);
                    btnfinalplot.setEnabled(true);
                    //edtMessage.setText("");
                } catch (UnsupportedEncodingException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }

            }
        });
        btnfinalplot.setOnClickListener(new View.OnClickListener()
        {
            public void onClick(View v)
            {

                Intent intent=new Intent(MainActivity.this,GraphActivity.class);
                startActivity(intent);
                graphplot1();


            }
        });
        
    }
    @Override
    protected void onResume() {
        super.onResume();
        // we're going to simulate real time with thread that append data to the graph
        new Thread(new Runnable() {

            @Override
            public void run() {
                // we add 100 new entries
                for (int i = 0; i < 100; i++) {
                    //Log.d(TAG, "I value in Main activity: "+i);
                    runOnUiThread(new Runnable() {

                        @Override
                        public void run() {
                            //graphplot1();
                        }
                    });

                    // sleep to slow down the add of entries
                    try {
                        Thread.sleep(600);
                    } catch (InterruptedException e) {
                        // manage error ...
                        Log.d(MainActivity.TAG,"Graph error:"+e);
                    }
                }
            }
        }).start();
    }

    public void graphplot1()
    {

        while(MainActivity.PayloadStatus[i]!=null)
        {
            Log.d(TAG, "Graph plot in main_activity activity: ");
            double x=MainActivity.stringId[i];

            double y = MainActivity.temperature[i];
            series1.appendData(new DataPoint(x, y), true, 10);
            i++;
        }
        Icount=i;
        Log.d(TAG, "Icount value: "+Icount);
        series1.setThickness(5);
        series1.setDrawDataPoints(true);
    }
    //UART service connected/disconnected
    private ServiceConnection mServiceConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder rawBinder) {
        		mService = ((UartService.LocalBinder) rawBinder).getService();
        		Log.d(TAG, "onServiceConnected mService= " + mService);
        		if (!mService.initialize()) {
                    Log.e(TAG, "Unable to initialize Bluetooth");
                    finish();
                }

        }

        public void onServiceDisconnected(ComponentName classname) {
       ////     mService.disconnect(mDevice);
        		mService = null;
        }
    };

    private Handler mHandler = new Handler() {
        @Override
        
        //Handler events that received from UART service 
        public void handleMessage(Message msg) {
  
        }
    };

    public int i=0;

    private final BroadcastReceiver UARTStatusChangeReceiver = new BroadcastReceiver() {

        public void onReceive(Context context, Intent intent) {

            String action = intent.getAction();

            final Intent mIntent = intent;
           //*********************//
            if (action.equals(UartService.ACTION_GATT_CONNECTED)) {
            	 runOnUiThread(new Runnable() {
                     public void run() {
                         	String currentDateTimeString = DateFormat.getTimeInstance().format(new Date());
                             Log.d(TAG, "UART_CONNECT_MSG");

                             btnConnectDisconnect.setText("Disconnect");
                             //edtMessage.setEnabled(true);
                            // btnSend.setEnabled(true);
                             ((TextView) findViewById(R.id.deviceName)).setText(mDevice.getName()+ " - ready");
                             listAdapter.add("["+currentDateTimeString+"] Connected to: "+ mDevice.getName());
                        	 	messageListView.smoothScrollToPosition(listAdapter.getCount() - 1);
                             mState = UART_PROFILE_CONNECTED;
                     }
            	 });
            }
           
          //*********************//
            if (action.equals(UartService.ACTION_GATT_DISCONNECTED)) {

            	 runOnUiThread(new Runnable() {
                     public void run() {
                    	 	 String currentDateTimeString = DateFormat.getTimeInstance().format(new Date());
                             Log.d(TAG, "UART_DISCONNECT_MSG");
                             btnConnectDisconnect.setText("Connect");
                             //edtMessage.setEnabled(false);
                             //btnSend.setEnabled(false);
                             ((TextView) findViewById(R.id.deviceName)).setText("Not Connected");
                             listAdapter.add("["+currentDateTimeString+"] Disconnected to: "+ mDevice.getName());
                             mState = UART_PROFILE_DISCONNECTED;
                             mService.close();
                            //setUiState();
                         
                     }
                 });
            }
            
          
          //*********************//
            if (action.equals(UartService.ACTION_GATT_SERVICES_DISCOVERED)) {
             	 mService.enableTXNotification();
            }
          //*********************//
            if (action.equals(UartService.ACTION_DATA_AVAILABLE)) {
              
                 final byte[] txValue = intent.getByteArrayExtra(UartService.EXTRA_DATA);

                 runOnUiThread(new Runnable() {
                     public void run() {
                         try {
                             String text = new String(txValue, "UTF-8");
                             String currentDateTimeString =DateFormat.getTimeInstance().format(new Date());
                             Log.d(TAG, "String now:" + text);
                             //DateFormat.getTimeInstance().format(new Date());
                             TextView t = (TextView)findViewById(R.id.temp);
                             TextView s = (TextView)findViewById(R.id.payload_status);
                             TextView e = (TextView)findViewById(R.id.elapsed_time);
                             if (text.contains("$"))
                             {
                                 $_count++;

                             }
                             if($_count==0||hash_count!=0) {
                                 string_array[string_i] = text;
                                 Log.d(TAG, "Array value now :" + string_array[string_i]);
                                 Log.d(TAG, "I value now:" + string_i);
                                 string_i++;
                                 Log.d(TAG, "I value after:" + string_i);
                                 Log.d(TAG, "Count value:" + string_count);

                                 if (string_count % 2 == 0) {
                                     Log.d(TAG, "string[i-1]:" + string_array[string_i - 1]);
                                     Log.d(TAG, "string[i]:" + string_array[string_i]);
                                     finaltext = string_array[string_i - 2] + string_array[string_i - 1];
                                     Log.d(TAG, "final text:" + finaltext);
                                     //listAdapter.add(finaltext);
                                 }
                                 string_count++;
                                 //String currentDateTimeString = DateFormat.getTimeInstance().format(new Date());
                                 //listAdapter.add("["+currentDateTimeString+"] RX: "+text);
                                 messageListView.smoothScrollToPosition(listAdapter.getCount() - 1);
                             }
                             else if ($_count!=0&&hash_count==0) {
                                 //Toast.makeText(this,"File received succesfully",Toast.LENGTH_SHORT).show();
                                 file_array[file_i] = text;
                                 Log.d(TAG, "file array: " + file_array[file_i]);
                                listAdapter.add("File text:" + file_array[file_i]);
                                 file_i++;
                                 if (text.contains("#")) {
                                     hash_count++;
                                 }
                             }

                             if (finaltext.contains("@")&&finaltext.contains("~")) {
                                 DateTime[i] = currentDateTimeString;
                                 Log.d(TAG, "The current datetime is:" + DateTime[i]);
                                 Log.d(TAG, "String now:" + string_array[i]);
                                 StringTokenizer tokens = new StringTokenizer(finaltext, ",");
                                 String flag = tokens.nextToken();
                                 stringId[i] = Double.parseDouble(flag.substring(1));
                                 e.setText(flag);
                                 Log.d(TAG, "String Id:" + stringId[i]);
                                 String temp = tokens.nextToken();
                                 temperature[i] = Double.parseDouble(temp);
                                 Log.d(TAG, "temperature:" + temperature[i]);
                                 t.setText(temp);
                                 listAdapter.add("Temperature inside the Box:" + temperature[i] + " C");
                                 PayloadStatus[i] = tokens.nextToken();
                                 s.setText(PayloadStatus[i]);
                                 Log.d(TAG, "Payload status:" + PayloadStatus[i]);
                                 Log.d(TAG, "length of the array:"+PayloadStatus.length);
                                 listAdapter.add("Payload Status:Good");
                                 listAdapter.add("\n\n\n");
                                 Box_Status[i] = tokens.nextToken();
                                 Time[i] = i;
                                 Log.d(TAG, "Time values is:"+Time[i]);
                                 i++;

                             }
                             messageListView.smoothScrollToPosition(listAdapter.getCount() - 1);

                         } catch (Exception e) {
                             Log.e(TAG, e.toString());
                         }

                     }
                 });
             }
           //*********************//
            if (action.equals(UartService.DEVICE_DOES_NOT_SUPPORT_UART)){
            	showMessage("Device doesn't support UART. Disconnecting");
            	mService.disconnect();
            }
            
            
        }
    };

    private void service_init() {

        Intent bindIntent = new Intent(this, UartService.class);
        bindService(bindIntent, mServiceConnection, Context.BIND_AUTO_CREATE);
  
        LocalBroadcastManager.getInstance(this).registerReceiver(UARTStatusChangeReceiver, makeGattUpdateIntentFilter());
    }
    private static IntentFilter makeGattUpdateIntentFilter() {
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(UartService.ACTION_GATT_CONNECTED);
        intentFilter.addAction(UartService.ACTION_GATT_DISCONNECTED);
        intentFilter.addAction(UartService.ACTION_GATT_SERVICES_DISCOVERED);
        intentFilter.addAction(UartService.ACTION_DATA_AVAILABLE);
        intentFilter.addAction(UartService.DEVICE_DOES_NOT_SUPPORT_UART);
        return intentFilter;
    }
    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onDestroy() {
    	 super.onDestroy();
        Log.d(TAG, "onDestroy()");
        
        try {
        	LocalBroadcastManager.getInstance(this).unregisterReceiver(UARTStatusChangeReceiver);
        } catch (Exception ignore) {
            Log.e(TAG, ignore.toString());
        } 
        unbindService(mServiceConnection);
        mService.stopSelf();
        mService= null;
       
    }

    @Override
    protected void onStop() {
        Log.d(TAG, "onStop");
        super.onStop();
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause");
        super.onPause();
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        Log.d(TAG, "onRestart");
    }

    //@Override
  /*  public void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
        if (!mBtAdapter.isEnabled()) {
            Log.i(TAG, "onResume - BT not enabled yet");
            Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
        }
 
    }
*/
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {

        switch (requestCode) {

        case REQUEST_SELECT_DEVICE:
        	//When the DeviceListActivity return, with the selected device address
            if (resultCode == Activity.RESULT_OK && data != null) {
                String deviceAddress = data.getStringExtra(BluetoothDevice.EXTRA_DEVICE);
                mDevice = BluetoothAdapter.getDefaultAdapter().getRemoteDevice(deviceAddress);
               
                Log.d(TAG, "... onActivityResultdevice.address==" + mDevice + "mserviceValue" + mService);
                ((TextView) findViewById(R.id.deviceName)).setText(mDevice.getName()+ " - connecting");
                mService.connect(deviceAddress);
                            

            }
            break;
        case REQUEST_ENABLE_BT:
            // When the request to enable Bluetooth returns
            if (resultCode == Activity.RESULT_OK) {
                Toast.makeText(this, "Bluetooth has turned on ", Toast.LENGTH_SHORT).show();

            } else {
                // User did not enable Bluetooth or an error occurred
                Log.d(TAG, "BT not enabled");
                Toast.makeText(this, "Problem in BT Turning ON ", Toast.LENGTH_SHORT).show();
                finish();
            }
            break;
        default:
            Log.e(TAG, "wrong request code");
            break;
        }
    }

    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId) {
       
    }

    
    private void showMessage(String msg) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
  
    }

    @Override
    public void onBackPressed() {
        if (mState == UART_PROFILE_CONNECTED) {
            Intent startMain = new Intent(Intent.ACTION_MAIN);
            startMain.addCategory(Intent.CATEGORY_HOME);
            startMain.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(startMain);
            showMessage("MaxQ App is running in background.\n             Disconnect to exit");
        }
        else {
            new AlertDialog.Builder(this)
            .setIcon(android.R.drawable.ic_dialog_alert)
            .setTitle(R.string.popup_title)
            .setMessage(R.string.popup_message)
            .setPositiveButton(R.string.popup_yes, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
   	                finish();
                }
            })
            .setNegativeButton(R.string.popup_no, null)
            .show();
        }
    }
}
