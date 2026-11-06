package com.cleiltonbr.moheuropeanassault

import android.app.Activity
import android.os.Bundle
import android.view.Gravity
import android.widget.TextView

class MainActivity : Activity() {
    private external fun nativeStatus(): String

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        System.loadLibrary("moh_android")
        val status = TextView(this).apply {
            text = "Medal of Honor: European Assault\n\nAndroid ARM64 bootstrap\n" + nativeStatus()
            textSize = 18f
            gravity = Gravity.CENTER
            setPadding(32, 32, 32, 32)
        }
        setContentView(status)
    }
}
