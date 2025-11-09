package com.karolbystrek.mqttdatahub.service;

import com.karolbystrek.mqttdatahub.model.Customer;
import com.karolbystrek.mqttdatahub.model.Location;
import com.karolbystrek.mqttdatahub.repository.LocationRepository;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;

import static java.lang.String.format;

@Service
@RequiredArgsConstructor
public class LocationService {

    private final LocationRepository locationRepository;

    public Location getLocationBy(Long locationId, Customer customer) {
        return locationRepository.findByIdAndCustomerId(locationId, customer.getId())
                .orElseThrow(() ->
                        new IllegalArgumentException(format("Location with id '%s' for customer '%s' not found.", locationId, customer.getName()))
                );
    }
}
