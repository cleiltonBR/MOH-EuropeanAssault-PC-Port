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
import android.widget.LinearLayout
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
    private external fun nativeConfigureLogging(directory: String)
    private external fun nativeRequestGameStart()

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
        nativeConfigureLogging(filesDir.absolutePath)
        Thread.setDefaultUncaughtExceptionHandler { thread, error ->
            appendLocalLog("FATAL JAVA EXCEPTION on " + thread.name + ": " + android.util.Log.getStackTraceString(error))
            val marker = File(filesDir, "moh-crash.marker")
            try { marker.writeText("java-crash\n") } catch (_: Exception) { }
            android.os.Process.killProcess(android.os.Process.myPid())
        }
        appendLocalLog("App: onCreate")
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

        val actions = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(8, 4, 8, 8)
        }
        val startButton = Button(this).apply {
            text = "INICIAR JOGO"
            setOnClickListener {
                appendLocalLog("UI: start button tapped")
                nativeRequestGameStart()
                Toast.makeText(this@MainActivity, "Inicialização do jogo solicitada.", Toast.LENGTH_SHORT).show()
            }
        }
        val importButton = Button(this).apply {
            text = "IMPORTAR PASTA DO JOGO"
            setOnClickListener {
                val intent = Intent(Intent.ACTION_OPEN_DOCUMENT_TREE).apply {
                    addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
                }
                startActivityForResult(intent, importRequest)
            }
        }
        val shareLogsButton = Button(this).apply {
            text = "COMPARTILHAR LOGS"
            setOnClickListener { shareDiagnostics() }
        }
        actions.addView(startButton)
        actions.addView(importButton)
        actions.addView(shareLogsButton)
        root.addView(actions, FrameLayout.LayoutParams(
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
            appendLocalLog("Import: copied game directory to " + gameDir.absolutePath)
            Toast.makeText(this, "Arquivos copiados. " +
                if (File(gameDir, "default.xbe").isFile) "default.xbe encontrado." else "default.xbe não encontrado.",
                Toast.LENGTH_LONG).show()
        } catch (e: Exception) {
            appendLocalLog("Import exception: " + android.util.Log.getStackTraceString(e))
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


    private fun appendLocalLog(message: String) {
        try {
            File(filesDir, "moh-runtime.log").appendText(
                java.text.SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", java.util.Locale.US).format(java.util.Date()) +
                    " " + message + "\n"
            )
        } catch (_: Exception) { }
    }

    private fun shareDiagnostics() {
        val logFile = File(filesDir, "moh-runtime.log")
        val marker = File(filesDir, "moh-crash.marker")
        val report = buildString {
            append("Medal of Honor: European Assault - Android diagnostic report\n")
            append("App version: 0.2.0-android-game-code-linked\n")
            append("Crash marker present: ").append(marker.exists()).append("\n\n")
            append("Runtime status: captured from local files; native runtime intentionally not queried.\n")
            append("\nRuntime log:\n")
            if (logFile.exists()) {
                try {
                    // Scan the complete log in two streaming passes: the fatal signal
                    // is written after the pre-entry memory snapshot, so a tail-only
                    // read can miss the mappings needed to resolve the crash addresses.
                    val addressPattern = Regex("(?:fault_addr|pc|sp|lr)=0x([0-9a-fA-F]+)")
                    val mapPattern = Regex("^([0-9a-fA-F]+)-([0-9a-fA-F]+)\\s")
                    val crashAddresses = mutableSetOf<ULong>()
                    java.io.RandomAccessFile(logFile, "r").use { input ->
                        while (true) {
                            val line = input.readLine() ?: break
                            if (line.contains("FATAL NATIVE SIGNAL:")) {
                                addressPattern.findAll(line).forEach { match ->
                                    match.groupValues[1].toULongOrNull(16)?.let(crashAddresses::add)
                                }
                            }
                        }
                    }

                    val selectedMaps = sortedSetOf<String>()
                    var inCrashSnapshot = false
                    java.io.RandomAccessFile(logFile, "r").use { input ->
                        while (true) {
                            val line = input.readLine() ?: break
                            when (line.trim()) {
                                "CRASH MAP SNAPSHOT BEGIN" -> inCrashSnapshot = true
                                "CRASH MAP SNAPSHOT END" -> inCrashSnapshot = false
                                else -> if (inCrashSnapshot) {
                                    val trimmed = line.trimStart()
                                    val match = mapPattern.find(trimmed)
                                    if (match != null) {
                                        val mapStart = match.groupValues[1].toULongOrNull(16)
                                        val mapEnd = match.groupValues[2].toULongOrNull(16)
                                        if (mapStart != null && mapEnd != null &&
                                            crashAddresses.any { it >= mapStart && it < mapEnd }) {
                                            selectedMaps.add(trimmed)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Keep the ordinary diagnostic tail bounded for Android Binder,
                    // then append matching mappings even when they precede that tail.
                    val maxLogBytes = 48 * 1024L
                    val length = logFile.length()
                    val startOffset = (length - maxLogBytes).coerceAtLeast(0L)
                    val tail = java.io.RandomAccessFile(logFile, "r").use { input ->
                        input.seek(startOffset)
                        val bytes = ByteArray((length - startOffset).toInt())
                        input.readFully(bytes)
                        String(bytes, Charsets.UTF_8)
                    }
                    if (startOffset > 0) append("[Earlier log content omitted; showing last 48 KiB.]\n")
                    tail.lineSequence()
                        .filter { line ->
                            val trimmed = line.trim()
                            !mapPattern.containsMatchIn(trimmed) &&
                                trimmed != "CRASH MAP SNAPSHOT BEGIN" &&
                                trimmed != "CRASH MAP SNAPSHOT END"
                        }
                        .forEach { append(it).append('\n') }

                    append("\nRelevant crash memory mappings (scanned from the complete log):\n")
                    if (selectedMaps.isEmpty()) {
                        append("No mapping contained fault_addr, PC, SP or LR.\n")
                    } else {
                        selectedMaps.forEach { append(it).append('\n') }
                    }
                } catch (e: Exception) {
                    append("Could not read runtime log: ").append(e.message)
                }
            }
            if (marker.exists()) {
                try { append("\n\nCrash marker:\n").append(marker.readText().take(1024)) }
                catch (e: Exception) { append("\nCrash marker could not be read: ").append(e.message) }
            }
        }
        val send = Intent(Intent.ACTION_SEND).apply {
            type = "text/plain"
            putExtra(Intent.EXTRA_SUBJECT, "Medal of Honor Android - crash diagnostics")
            putExtra(Intent.EXTRA_TEXT, report)
        }
        try {
            startActivity(Intent.createChooser(send, "Compartilhar diagnóstico"))
        } catch (e: Throwable) {
            appendLocalLog("Diagnostics share failed: " + android.util.Log.getStackTraceString(e))
            Toast.makeText(this, "Não foi possível abrir o compartilhamento: ${e.message}", Toast.LENGTH_LONG).show()
        }
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
