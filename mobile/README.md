# Mobile App Setup & Build Guide

This guide explains how to set up the development environment, run the app in "Dev Mode" (using Expo Go), and build the standalone APK file for Android.

## 1. Prerequisites

Before you start, ensure you have the following installed on your computer:

*   **Node.js** (LTS version recommended): [Download Here](https://nodejs.org/)
*   **Git**: [Download Here](https://git-scm.com/)
*   **Expo CLI**: Install globally via terminal:
    ```bash
    npm install -g eas-cli
    ```
*   **Expo Go App**: Install this on your Android phone from the Google Play Store.

## 2. Project Setup

1.  Open a terminal/command prompt.
2.  Navigate to the `mobile` directory inside the project:
    ```bash
    cd path/to/ESP32_smart_led/mobile
    ```
3.  Install dependencies:
    ```bash
    npm install
    ```

## 3. Running in Dev Mode (Custom Dev Client)

**Important**: Because this app uses **Bluetooth (BLE)**, it **cannot** run in the standard "Expo Go" app from the Play Store. You must build a custom "Development Client" once.

### Phase A: Build & Install the Dev Client (Do this ONCE)

1.  **Run the Build Command**:
    ```bash
    eas build -p android --profile development
    ```
    *(Log in with your Expo account if prompted)*

2.  **Install on Phone**:
    *   Wait for the build to finish.
    *   Scan the QR code to download and install the `.apk`.
    *   This app allows you to develop just like Expo Go, but with Bluetooth support enabled.

### Phase B: Daily Development (Do this every time)

1.  **Check IP Address** (mobile/App.js):
    *   Replace `172.20.10.7` with your computer's local Wi-Fi IP.

2.  **Start the Server**:
    ```bash
    npx expo start --dev-client
    ```

3.  **Run on Device**:
    *   Scan the QR code *using your camera or the helper instructions*.
    *   It should open your specific **"Smart LED Control"** app (Dev Mode), not Expo Go.
    *   Changes to code will reload instantly.

## 4. Building the Final App (APK)

When you are ready to create a standalone app file (`.apk`) that works without Expo Go (and without the development server running on your computer, provided the backend is accessible/cloud-hosted, OR if you are still just testing on local wifi):

1.  **Configure Build Profile** (Done):
    *   The `eas.json` file is already configured with a `preview` profile that builds an APK.

2.  **Run Build Command**:
    ```bash
    eas build -p android --profile preview
    ```
    *   You may need to log in with an Expo account (`eas login`).

3.  **Install on Phone**:
    *   Wait for the build to finish (can take 10-15 mins).
    *   Scan the QR code provided at the end of the build process to download the `.apk`.
    *   Install the file on your phone.

## Troubleshooting

*   **"Network Error" or Backend not reachable**: Ensure `App.js` references the correct `http://<YOUR_IP>:8080` and that the Spring Boot backend is running.
*   **Metro Bundler Issues**: Press `r` in the terminal to reload the bundle.
