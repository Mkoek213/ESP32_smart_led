package com.karolbystrek.mqttdatahub.service;

import com.karolbystrek.mqttdatahub.model.Customer;
import com.karolbystrek.mqttdatahub.repository.CustomerRepository;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;

import static java.lang.String.format;

@Service
@RequiredArgsConstructor
public class CustomerService {

    private final CustomerRepository customerRepository;

    public Customer getBy(Long customerId) {
        return customerRepository.findById(customerId)
                .orElseThrow(() -> new IllegalArgumentException(format("Customer with id '%s' not found.", customerId)));
    }
}
