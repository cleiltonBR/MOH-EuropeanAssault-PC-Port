package com.cleiltonbr.moheuropeanassault

import android.app.Activity
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.FrameLayout
import android.widget.TextView

class MainActivity : Activity(), SurfaceHolder.Callback {
    private external fun nativeStatus(): String
    private external fun nativeStart(surface: Surface)
    private external fun nativeStop()

    private lateinit var status: TextView
    private val handler = Handler(Looper.getMainLooper())
    private val refreshStatus = object : Runnable {
        override fun run() {
            if (::status.isInitialized) status.text = "Medal of Honor: European Assault\\n\\n" + nativeStatus()
            handler.postDelayed(this, 500)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        System.loadLibrary("moh_android")

        val root = FrameLayout(this)
        val rendererSurface = SurfaceView(this)
        rendererSurface.holder.addCallback(this)
        root.addView(
            rendererSurface,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
            )
        )

        status = TextView(this).apply {
            text = "Medal of Honor: European Assault\n\nGLES3 renderer validation\n" + nativeStatus()
            textSize = 14f
            gravity = Gravity.CENTER
            setPadding(24, 16, 24, 16)
            setBackgroundColor(0x99000000.toInt())
        }
        val statusParams = FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.WRAP_CONTENT,
            Gravity.TOP
        )
        root.addView(status, statusParams)
        setContentView(root)
        handler.post(refreshStatus)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        nativeStart(holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        nativeStop()
        nativeStart(holder.surface)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        nativeStop()
    }

    override fun onDestroy() {
        handler.removeCallbacks(refreshStatus)
        nativeStop()
        super.onDestroy()
    }
}
