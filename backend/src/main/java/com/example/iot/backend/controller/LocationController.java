package com.example.iot.backend.controller;

import com.example.iot.backend.dto.location.LocationCreateRequest;
import com.example.iot.backend.dto.location.LocationResponse;
import com.example.iot.backend.model.User;
import com.example.iot.backend.service.LocationService;
import jakarta.validation.Valid;
import lombok.RequiredArgsConstructor;
import org.springframework.http.HttpStatus;
import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.core.userdetails.UserDetails;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

@RequiredArgsConstructor
@RequestMapping("/locations")
@RestController
public class LocationController {

    private final LocationService locationService;

    @GetMapping
    public List<LocationResponse> getAllLocations(@AuthenticationPrincipal UserDetails userDetails) {
        return locationService.getAllLocations(getUserId(userDetails));
    }

    @GetMapping("/search")
    public List<LocationResponse> getLocationsByName(@RequestParam String name, @AuthenticationPrincipal UserDetails userDetails) {
        return locationService.searchLocations(getUserId(userDetails), name);
    }

    @PostMapping
    @ResponseStatus(HttpStatus.CREATED)
    public LocationResponse createLocation(@RequestBody @Valid LocationCreateRequest request, @AuthenticationPrincipal UserDetails userDetails) {
        return locationService.createLocation(getUserId(userDetails), request);
    }

    @PutMapping("/{locationId}")
    public LocationResponse updateLocation(@PathVariable Long locationId, @RequestBody @Valid LocationCreateRequest request, @AuthenticationPrincipal UserDetails userDetails) {
        return locationService.updateLocation(getUserId(userDetails), locationId, request);
    }

    @DeleteMapping("/{locationId}")
    @ResponseStatus(HttpStatus.NO_CONTENT)
    public void deleteLocation(@PathVariable Long locationId, @AuthenticationPrincipal UserDetails userDetails) {
        locationService.deleteLocation(getUserId(userDetails), locationId);
    }


    private Long getUserId(UserDetails userDetails) {
        if (userDetails instanceof User user) {
            return user.getId();
        }
        throw new IllegalArgumentException("UserDetails is not an instance of User");
    }
}
