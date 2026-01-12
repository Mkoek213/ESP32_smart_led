package com.example.iot.backend.service;

import com.example.iot.backend.model.FirmwareUpdate;
import com.example.iot.backend.repository.FirmwareRepository;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.multipart.MultipartFile;
import org.springframework.util.DigestUtils;

import java.io.IOException;

@Slf4j
@Service
@RequiredArgsConstructor
public class FirmwareService {

    private final FirmwareRepository firmwareRepository;

    @Transactional
    public FirmwareUpdate storeFirmware(MultipartFile file, String version) throws IOException {
        log.info("Storing new firmware version: {}", version);

        String checksum = DigestUtils.md5DigestAsHex(file.getInputStream());

        FirmwareUpdate firmware = FirmwareUpdate.builder()
                .version(version)
                .filename(file.getOriginalFilename())
                .data(file.getBytes())
                .checksum(checksum)
                .build();

        return firmwareRepository.save(firmware);
    }

    @Transactional(readOnly = true)
    public FirmwareUpdate getLatestFirmware() {
        return firmwareRepository.findFirstByOrderByCreatedAtDesc().orElse(null);
    }

    @Transactional(readOnly = true)
    public FirmwareUpdate getFirmwareByVersion(String version) {
        return firmwareRepository.findByVersion(version).orElse(null);
    }
}
