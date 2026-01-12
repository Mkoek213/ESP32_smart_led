package com.example.iot.backend.controller;

import com.example.iot.backend.model.FirmwareUpdate;
import com.example.iot.backend.service.FirmwareService;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.core.io.ByteArrayResource;
import org.springframework.core.io.Resource;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;

import java.io.IOException;

@Slf4j
@RestController
@RequestMapping("/firmware")
@RequiredArgsConstructor
@CrossOrigin(origins = "*", allowedHeaders = "*", methods = { RequestMethod.GET, RequestMethod.POST,
        RequestMethod.OPTIONS })
public class FirmwareController {

    private final FirmwareService firmwareService;

    @PostMapping("/upload")
    public ResponseEntity<?> uploadFirmware(@RequestParam("file") MultipartFile file,
            @RequestParam("version") String version) {
        try {
            firmwareService.storeFirmware(file, version);
            return ResponseEntity.ok().body("Firmware uploaded successfully");
        } catch (IOException e) {
            log.error("Failed to upload firmware", e);
            return ResponseEntity.internalServerError().body("Failed to upload firmware");
        } catch (Exception e) {
            log.error("Error saving firmware", e);
            return ResponseEntity.badRequest().body("Error: " + e.getMessage());
        }
    }

    @GetMapping("/check")
    public ResponseEntity<?> checkForUpdate(@RequestParam("version") String currentVersion) {
        FirmwareUpdate latest = firmwareService.getLatestFirmware();
        if (latest != null && !latest.getVersion().equals(currentVersion)) {
            log.info("New firmware available: {} (Current: {})", latest.getVersion(), currentVersion);
            String url = org.springframework.web.servlet.support.ServletUriComponentsBuilder.fromCurrentContextPath()
                    .path("/firmware/download/{version}")
                    .buildAndExpand(latest.getVersion())
                    .toUriString();
            return ResponseEntity.ok().body(new FirmwareUpdateResponse(latest.getVersion(), url));
        }
        return ResponseEntity.status(304).build(); // Not Modified
    }

    @GetMapping("/download/{version}")
    public ResponseEntity<Resource> downloadFirmware(@PathVariable String version) {
        FirmwareUpdate firmware = firmwareService.getFirmwareByVersion(version);
        if (firmware == null) {
            return ResponseEntity.notFound().build();
        }

        return ResponseEntity.ok()
                .contentType(MediaType.APPLICATION_OCTET_STREAM)
                .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=\"" + firmware.getFilename() + "\"")
                .body(new ByteArrayResource(firmware.getData()));
    }

    record FirmwareUpdateResponse(String version, String url) {
    }
}
