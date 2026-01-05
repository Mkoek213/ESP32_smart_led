package com.example.iot.backend.exception;

import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.ResponseStatus;

@ResponseStatus(HttpStatus.NOT_FOUND)
public class ResourceNotFoundException extends RuntimeException {
    public ResourceNotFoundException(String message) {
        super(message);
    }

    public static ResourceNotFoundException throwResourceNotFound(Class<?> clazz) {
        String message = String.format("%s not found", clazz.getSimpleName());
        return new ResourceNotFoundException(message);
    }
}
