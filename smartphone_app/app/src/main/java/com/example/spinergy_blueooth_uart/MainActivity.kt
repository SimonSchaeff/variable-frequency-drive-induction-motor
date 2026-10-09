package com.example.spinergy_blueooth_uart

import android.Manifest
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import com.example.spinergy_blueooth_uart.ui.theme.Spinergy_Blueooth_UARTTheme
import java.util.*

class MainActivity : ComponentActivity() {

    private val bluetoothAdapter: BluetoothAdapter? by lazy {
        val bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothManager.adapter
    }

    private var bluetoothGatt: BluetoothGatt? = null
    private var writeCharacteristic: BluetoothGattCharacteristic? = null

    // Common UART Service/Characteristic UUIDs (Nordic NUS, HM-10, etc.)
    private val UART_WRITE_UUIDS = listOf(
        UUID.fromString("6e400002-b5a3-f393-e0a9-e50e24dcca9e"), // Nordic NUS
        UUID.fromString("0000ffe1-0000-1000-8000-00805f9b34fb"), // HM-10 / CC2541
        UUID.fromString("0000ffe9-0000-1000-8000-00805f9b34fb")  // Other common serial
    )

    private val discoveredDevices = mutableStateListOf<BluetoothDevice>()
    private val isScanning = mutableStateOf(false)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            Spinergy_Blueooth_UARTTheme {
                MainScreen(
                    discoveredDevices = discoveredDevices,
                    isScanning = isScanning.value,
                    onScan = { startScan() },
                    onConnect = { device -> connectToDevice(device) },
                    onSendMessage = { value -> sendMessage(value) }
                )
            }
        }
    }

    private fun startScan() {
        if (bluetoothAdapter == null) return
        
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(this, "Scan permission missing", Toast.LENGTH_SHORT).show()
            return
        }

        discoveredDevices.clear()
        isScanning.value = true
        
        val scanner = bluetoothAdapter!!.bluetoothLeScanner
        scanner.startScan(scanCallback)
        
        // Stop scan after 5 seconds
        Handler(Looper.getMainLooper()).postDelayed({
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED) {
                scanner.stopScan(scanCallback)
                isScanning.value = false
            }
        }, 5000)
    }

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            if (!discoveredDevices.any { it.address == result.device.address }) {
                discoveredDevices.add(result.device)
            }
        }
    }

    private fun connectToDevice(device: BluetoothDevice) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) return

        bluetoothGatt?.disconnect()
        bluetoothGatt?.close()

        runOnUiThread { Toast.makeText(this, "Connecting...", Toast.LENGTH_SHORT).show() }
        bluetoothGatt = device.connectGatt(this, false, gattCallback)
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                Log.d("BLE", "Connected to GATT server.")
                if (ContextCompat.checkSelfPermission(this@MainActivity, Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) {
                    gatt.discoverServices()
                }
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                Log.d("BLE", "Disconnected from GATT server.")
                runOnUiThread { Toast.makeText(this@MainActivity, "Disconnected", Toast.LENGTH_SHORT).show() }
                writeCharacteristic = null
                bluetoothGatt?.close()
                bluetoothGatt = null
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.d("BLE", "Services discovered.")
                findUartCharacteristic(gatt)
            }
        }
    }

    private fun findUartCharacteristic(gatt: BluetoothGatt) {
        for (service in gatt.services) {
            for (char in service.characteristics) {
                // Check if it's a known UART UUID or if it has write properties
                val isKnownUart = UART_WRITE_UUIDS.contains(char.uuid)
                val canWrite = (char.properties and BluetoothGattCharacteristic.PROPERTY_WRITE != 0) ||
                               (char.properties and BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE != 0)
                
                if (isKnownUart || canWrite) {
                    writeCharacteristic = char
                    runOnUiThread { Toast.makeText(this, "UART Ready!", Toast.LENGTH_SHORT).show() }
                    Log.d("BLE", "Found Write Characteristic: ${char.uuid}")
                    return
                }
            }
        }
        runOnUiThread { Toast.makeText(this, "UART Service not found on this device", Toast.LENGTH_LONG).show() }
    }

    private fun sendMessage(value: Int) {
        val char = writeCharacteristic ?: run {
            Toast.makeText(this, "Not connected to a UART device", Toast.LENGTH_SHORT).show()
            return
        }

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) return

        // Umwandlung von Int (z.B. 50) zu String + Newline (z.B. "50\n")
        // Das ist notwendig, da dein STM32-Code 'strtol' verwendet und auf '\n' wartet.
        val message = value.toString() + "\n"
        val data = message.toByteArray()
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            bluetoothGatt?.writeCharacteristic(char, data, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
        } else {
            @Suppress("DEPRECATION")
            char.value = data
            @Suppress("DEPRECATION")
            bluetoothGatt?.writeCharacteristic(char)
        }
        Log.d("BLE", "Sent string: $message")
    }

    override fun onDestroy() {
        super.onDestroy()
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) {
            bluetoothGatt?.disconnect()
            bluetoothGatt?.close()
        }
    }
}

@Composable
fun MainScreen(
    discoveredDevices: List<BluetoothDevice>,
    isScanning: Boolean,
    onScan: () -> Unit,
    onConnect: (BluetoothDevice) -> Unit,
    onSendMessage: (Int) -> Unit
) {
    var sliderValue by remember { mutableFloatStateOf(0f) }
    val permissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { _ -> }

    LaunchedEffect(Unit) {
        permissionLauncher.launch(arrayOf(
            Manifest.permission.BLUETOOTH_SCAN,
            Manifest.permission.BLUETOOTH_CONNECT,
            Manifest.permission.ACCESS_FINE_LOCATION
        ))
    }

    Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
        Column(
            modifier = Modifier
                .padding(innerPadding)
                .fillMaxSize()
                .padding(16.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Spacer(modifier = Modifier.height(24.dp))
            Text(
                text = "Slider Value: ${sliderValue.toInt()}",
                style = MaterialTheme.typography.headlineMedium
            )
            Slider(
                value = sliderValue,
                onValueChange = { sliderValue = it },
                valueRange = 0f..99f,
                modifier = Modifier.padding(horizontal = 16.dp)
            )
            Button(
                onClick = { onSendMessage(sliderValue.toInt()) },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("Send Value")
            }

            Spacer(modifier = Modifier.height(32.dp))
            HorizontalDivider()
            Spacer(modifier = Modifier.height(16.dp))

            Button(
                onClick = onScan,
                enabled = !isScanning,
                modifier = Modifier.fillMaxWidth()
            ) {
                Text(if (isScanning) "Scanning..." else "Scan for BLE Devices")
            }

            Spacer(modifier = Modifier.height(8.dp))

            LazyColumn(modifier = Modifier.weight(1f)) {
                items(discoveredDevices) { device ->
                    ListItem(
                        headlineContent = {
                            val deviceName = try { device.name ?: "Unknown Device" } catch (e: SecurityException) { "Unknown Device" }
                            Text(deviceName)
                        },
                        supportingContent = { Text(device.address) },
                        trailingContent = {
                            Button(onClick = { onConnect(device) }) {
                                Text("Connect")
                            }
                        }
                    )
                    HorizontalDivider()
                }
            }
        }
    }
}
