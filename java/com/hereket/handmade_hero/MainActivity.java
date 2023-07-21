package com.hereket.handmade_hero;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.view.View;
import android.util.Log;
import android.graphics.Canvas;
import android.content.Context;
import android.graphics.Paint;
import android.graphics.Color;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.graphics.BitmapFactory;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.graphics.Matrix;
import android.content.res.Resources;
import android.view.MotionEvent;
import android.media.AudioTrack;
import android.media.AudioManager;
import android.media.AudioFormat;
import android.media.AudioTrack.OnPlaybackPositionUpdateListener;
import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.ActivityInfo;
import android.app.ActivityManager;
import android.app.ActivityManager.MemoryInfo;
import android.content.res.AssetManager;

import android.view.WindowManager;



public class MainActivity extends Activity {
    private String TAG = "------ HERO:";

    Bitmap drawingBitmap;

    int bitmapWidth;
    int bitmapHeight;
    MyState mystate;

    // AudioTrack audioTrack;

    static {
        System.loadLibrary("android_handmade");
    }

    public native void drawStuff(Bitmap bitmap, int width, int height, MyState state);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);
        // setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        // getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

        super.onCreate(savedInstanceState);
        View view = new MySurfaceView(this);
        setContentView(view);
    }

    //--------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------
    
    class MyState {
        public float touchX;
        public float touchY;

        public int canvasWidth;
        public int canvasHeight;
        public double dtForFrame;

        public String packageDirectory;

        public AssetManager assetManager;
    }

    public class MySurfaceView extends SurfaceView implements SurfaceHolder.Callback, View.OnTouchListener {
        private DrawThread drawThread;

        public MySurfaceView(Context context) {
            super(context);
            getHolder().addCallback(this);
            this.setOnTouchListener(this);
            mystate = new MyState();


            PackageManager packageManager = getPackageManager();
            String packageName = getPackageName();
            try {
                PackageInfo info = packageManager.getPackageInfo(packageName, 0);
                mystate.packageDirectory = info.applicationInfo.dataDir;
            }catch (Exception e) {
            }

            mystate.assetManager = getResources().getAssets();

            try{
                String[] test = mystate.assetManager.list("test");
                for(int i = 0; i < test.length; i++) {
                    // Log.d(TAG, String.format("...----------- %s", test[i]));
                }
            } catch(Exception e) {
            }

        }

        @Override
        public boolean onTouch(View v, MotionEvent event) {
            float X = event.getX();
            float Y = event.getY();

            // mystate.touchX = X;
            // mystate.touchY = Y;

            mystate.touchX = (X / mystate.canvasWidth) * bitmapWidth;
            mystate.touchY = (Y / mystate.canvasHeight) * bitmapHeight;

            // if (event.getAction() == MotionEvent.ACTION_DOWN) {
            // } 

            if (event.getAction() == MotionEvent.ACTION_UP) {
                mystate.touchX = -1;
                mystate.touchY = -1;
            }

            this.invalidate();
            return true;
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {	
            Bitmap.Config bitmapConfig = Bitmap.Config.ARGB_8888;

            double d = 1.77;

            mystate.canvasWidth = (int)(height * d);
            mystate.canvasHeight = height;

            int sizeDownMultiplier = 2;

            if(mystate.canvasWidth < 720 ||  mystate.canvasHeight < 720) {
                sizeDownMultiplier = 1;
            }

            bitmapWidth = 960;
            bitmapHeight = 540;

            drawingBitmap = Bitmap.createBitmap(bitmapWidth, bitmapHeight, bitmapConfig);
            drawingBitmap.eraseColor(Color.parseColor("#000000"));
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            drawThread = new DrawThread(getHolder());
            drawThread.setRunning(true);
            drawThread.start();
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            boolean retry = true;
            drawThread.setRunning(false);
            while (retry) {
                try {
                    drawThread.join();
                    retry = false;
                } catch (InterruptedException e) {
                    // TODO: catch error
                }
            }
        }
    }


    //--------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------

    class DrawThread extends Thread {
        private boolean runFlag = false;
        private SurfaceHolder surfaceHolder;
        private Matrix matrix;
        private long prevTime;
        private double targetSecondsPerFrame;

        public DrawThread(SurfaceHolder surfaceHolder){
            this.surfaceHolder = surfaceHolder;
            prevTime = System.nanoTime();

            int updateHertz = 30;
            targetSecondsPerFrame = 1.0 / (double)updateHertz;
        }

        public void setRunning(boolean run) {
            runFlag = run;
        }

        protected double getSecondsElapsed(long startNanoseconds, long endNanoseconds) {
            double result = -3.1234;
            long elapsedNanoseconds = endNanoseconds - startNanoseconds;
            double BILLION = 1_000_000_000;
            result = (double)elapsedNanoseconds / BILLION;
            return result;
        }

        @Override
        public void run() {
            Canvas canvas;
            double elapsedTime = 0;
            double timeToSleep = 0;

            Paint paint = new Paint();
            paint.setColor(Color.WHITE);
            paint.setTextSize(30);



            MemoryInfo mi = new MemoryInfo();
            ActivityManager activityManager = (ActivityManager) getSystemService(ACTIVITY_SERVICE);


            while (runFlag) {
                long startTime = System.nanoTime();
                canvas = null;
                mystate.dtForFrame = targetSecondsPerFrame;

            activityManager.getMemoryInfo(mi);
            double availableMegs = mi.availMem / 1_000_000L;
            double totalMemory = mi.totalMem / 1_000_000L;

                try {
                    canvas = surfaceHolder.lockCanvas();

                    if(canvas != null) {
                        synchronized (surfaceHolder) {
                            drawStuff(drawingBitmap, bitmapWidth, bitmapHeight, mystate);

                            canvas.drawColor(Color.RED);

                            canvas.drawBitmap(drawingBitmap, 
                                    new Rect(0, 0, bitmapWidth, bitmapHeight), 
                                    new Rect(0, 0, mystate.canvasWidth, mystate.canvasHeight), 
                                    null);

                            int lineHeight = 36;
                            int xoffset = 20;
                            canvas.drawText(String.format("Total: %.2f, Available: %.2f", totalMemory, availableMegs), xoffset, lineHeight, paint);
                            canvas.drawText(String.format("Elapsed Time: %f", elapsedTime), xoffset, lineHeight*2, paint);
                        }
                    }
                }
                finally {
                    if(canvas != null) {
                        surfaceHolder.unlockCanvasAndPost(canvas);
                    }
                }

                elapsedTime = getSecondsElapsed(startTime, System.nanoTime());
                if(elapsedTime < targetSecondsPerFrame) {
                    try{
                        timeToSleep = targetSecondsPerFrame - elapsedTime;
                        Thread.sleep((long)timeToSleep);
                    } catch(InterruptedException e) {
                        e.printStackTrace();
                    }
                } else {
                        timeToSleep = 0;
                }

            }
        }
    }
}
