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

    public Location getLocationBy(String locationName, Customer customer) {
        return locationRepository.findByNameAndCustomerId(locationName, customer.getId())
                .orElseThrow(() ->
                    new IllegalArgumentException(format("Location with name '%s' for customer '%s' not found.", locationName, customer.getName()))
                );
    }
}
