package com.cleiltonbr.moheuropeanassault

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.DocumentsContract
import android.database.Cursor
import android.view.Gravity
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.Button
import android.widget.FrameLayout
import android.widget.TextView
import android.widget.Toast
import java.io.File
import java.io.FileOutputStream

class MainActivity : Activity(), SurfaceHolder.Callback {
    private external fun nativeStatus(): String
    private external fun nativeStart(surface: Surface)
    private external fun nativeStop()
    private external fun nativeShutdown()
    private external fun nativeSetGameDirectory(path: String)

    private lateinit var status: TextView
    private val handler = Handler(Looper.getMainLooper())
    private val importRequest = 401
    private val refreshStatus = object : Runnable {
        override fun run() {
            if (::status.isInitialized) status.text = "Medal of Honor: European Assault\n\n" + nativeStatus()
            handler.postDelayed(this, 700)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        System.loadLibrary("moh_android")
        nativeSetGameDirectory(File(filesDir, "game").absolutePath)

        val root = FrameLayout(this)
        val rendererSurface = SurfaceView(this)
        rendererSurface.holder.addCallback(this)
        root.addView(rendererSurface, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT))

        status = TextView(this).apply {
            text = "Medal of Honor: European Assault\n\nImporte a pasta do jogo para preparar os arquivos."
            textSize = 14f
            gravity = Gravity.CENTER
            setPadding(24, 16, 24, 16)
            setBackgroundColor(0x99000000.toInt())
        }
        root.addView(status, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT, Gravity.TOP))

        val importButton = Button(this).apply {
            text = "IMPORTAR PASTA DO JOGO"
            setOnClickListener {
                val intent = Intent(Intent.ACTION_OPEN_DOCUMENT_TREE).apply {
                    addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
                }
                startActivityForResult(intent, importRequest)
            }
        }
        root.addView(importButton, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM))
        setContentView(root)
        handler.post(refreshStatus)
    }

    @Deprecated("Deprecated in Android API; retained for broad device compatibility")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != importRequest || resultCode != RESULT_OK) return
        val tree = data?.data ?: return
        try {
            contentResolver.takePersistableUriPermission(tree, Intent.FLAG_GRANT_READ_URI_PERMISSION)
            val gameDir = File(filesDir, "game")
            gameDir.deleteRecursively()
            gameDir.mkdirs()
            val rootId = DocumentsContract.getTreeDocumentId(tree)
            copyTreeDocument(tree, rootId, gameDir)
            nativeSetGameDirectory(gameDir.absolutePath)
            Toast.makeText(this, "Arquivos copiados. " +
                if (File(gameDir, "default.xbe").isFile) "default.xbe encontrado." else "default.xbe não encontrado.",
                Toast.LENGTH_LONG).show()
        } catch (e: Exception) {
            Toast.makeText(this, "Falha ao importar: ${e.message}", Toast.LENGTH_LONG).show()
        }
    }

    private fun copyTreeDocument(tree: Uri, documentId: String, destination: File) {
        val children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, documentId)
        val columns = arrayOf(DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_DISPLAY_NAME, DocumentsContract.Document.COLUMN_MIME_TYPE)
        contentResolver.query(children, columns, null, null, null)?.use { cursor: Cursor ->
            val idCol = cursor.getColumnIndexOrThrow(DocumentsContract.Document.COLUMN_DOCUMENT_ID)
            val nameCol = cursor.getColumnIndexOrThrow(DocumentsContract.Document.COLUMN_DISPLAY_NAME)
            val mimeCol = cursor.getColumnIndexOrThrow(DocumentsContract.Document.COLUMN_MIME_TYPE)
            while (cursor.moveToNext()) {
                val childId = cursor.getString(idCol)
                val safeName = File(cursor.getString(nameCol)).name
                if (safeName.isBlank() || safeName == "." || safeName == "..") continue
                val output = File(destination, safeName)
                if (cursor.getString(mimeCol) == DocumentsContract.Document.MIME_TYPE_DIR) {
                    output.mkdirs()
                    copyTreeDocument(tree, childId, output)
                } else {
                    val source = DocumentsContract.buildDocumentUriUsingTree(tree, childId)
                    contentResolver.openInputStream(source)?.use { input ->
                        FileOutputStream(output).use { out -> input.copyTo(out) }
                    }
                }
            }
        } ?: throw IllegalStateException("Não foi possível ler a pasta selecionada")
    }

    override fun surfaceCreated(holder: SurfaceHolder) { nativeStart(holder.surface) }
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        nativeStop()
        nativeStart(holder.surface)
    }
    override fun surfaceDestroyed(holder: SurfaceHolder) { nativeStop() }

    override fun onDestroy() {
        handler.removeCallbacks(refreshStatus)
        nativeStop()
        nativeShutdown()
        super.onDestroy()
    }
}
