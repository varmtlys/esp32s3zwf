# ESP32-S3 Zero WorkFlow Configuration Guide

This guide provides detailed instructions on how to configure your device and set up an external server for receiving webhooks.

---

## ⚙ Wi-Fi Configuration

When the device starts and cannot find a saved network, it enters **Access Point (AP) Mode**.

- **SSID**: `ESP32-WorkFlow-Config`
- **Password**: `12345678`
- **Default IP**: `192.168.4.1`

Once connected, you can configure the settings in `wifi.json`:
- `ssid`: Your Wi-Fi network name.
- `pass`: Your Wi-Fi password.

The device will automatically restart after saving these settings to connect to your network.

---

## ↩ Tunneling Configuration

Tunneling allows your ESP32-S3 to receive webhooks from the internet even if it is behind NAT or a firewall (no public IP required).

### `tunnel.json` Parameters:
- `enabled`: `true`/`false` — Enable or disable the tunnel client.
- `host`: The domain or public IP of your external proxy server.
- `port`: The port your proxy server is listening on for tunnel connections (e.g., `9000`).
- `protocol`: `http` or `https` (used for displaying the generated webhook URL in the UI).

---

## ⇄ External Server Setup (Caddy + socat)

To receive external webhooks, you need a public server (VPS) with a static IP. We will use **Caddy** for automated HTTPS/SSL and **socat** to bridge the traffic to the ESP32.

### Step-by-Step Setup:

#### 1. Install Caddy and socat
On your Linux server (Ubuntu/Debian):
```bash
sudo apt install -y caddy socat
```

#### 2. Configure Caddy
Create or edit your `Caddyfile` (usually in `/etc/caddy/Caddyfile`):
```caddy
your-domain.com {
    reverse_proxy localhost:8080
}
```
*Caddy will automatically handle SSL certificates for your domain.*

#### 3. Run the socat Tunnel
Run the following command to link the HTTP port (8080) to the ESP32 tunnel port (9000):
```bash
socat TCP-LISTEN:8080,reuseaddr,fork TCP-LISTEN:9000,reuseaddr
```

**How it works:**
1. **ESP32** connects to your server on port `9000` and keeps the connection open.
2. **Caddy** receives an HTTPS request on port `443` and proxies it to `localhost:8080`.
3. **socat** takes the traffic from `8080` and pipes it into the active connection on `9000`.
4. The request reaches the **ESP32**, which processes the flow and returns the response.

> [!TIP]
> To keep `socat` running in the background, use `screen`, `tmux`, or a systemd service.

---

## 🜁 OTA Updates Configuration

The device can check for and install firmware updates from a remote HTTP server.

### `ota.json` Parameters:
- `url`: Base URL where your firmware files and version info are hosted.

### Server-side File Structure:
1. `version.json`:
   ```json
   {
     "version": "1.0.1",
     "url": "http://yourserver.com/firmware_v101.bin"
   }
   ```
2. The firmware binary file (`.bin`).

When you click "Check Update" in the UI, the device compares its `FW_VERSION` with the one in `version.json`.

---

## 💾 File System Structure (LittleFS)

All data is stored in the internal flash memory:
- `/flows/` — Contains all your workflow scenarios in `.json` format.
- `/tables/` — JSON files used for simple data storage (local database).
- `/wifi.json`, `/tunnel.json`, `/ota.json` — System configuration files.

> [!WARNING]
> Clearing LittleFS during flashing will delete all your flows. Always back up your `/flows` folder via the web interface.
