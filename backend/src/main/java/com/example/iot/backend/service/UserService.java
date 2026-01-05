package com.example.iot.backend.service;

import com.example.iot.backend.dto.user.UserResponse;
import com.example.iot.backend.dto.user.UserUpdateRequest;
import com.example.iot.backend.model.User;
import com.example.iot.backend.repository.UserRepository;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

import static com.example.iot.backend.exception.ResourceNotFoundException.resourceNotFound;

@RequiredArgsConstructor
@Service
public class UserService {

    private final UserRepository userRepository;

    @Transactional(readOnly = true)
    public List<UserResponse> getAllUsers() {
        return userRepository.findAll().stream()
                .map(UserResponse::toUserResponse)
                .toList();
    }

    @Transactional(readOnly = true)
    public UserResponse getUser(Long userId) {
        return userRepository.findById(userId)
                .map(UserResponse::toUserResponse)
                .orElseThrow(() -> resourceNotFound(User.class));
    }

    @Transactional
    public UserResponse updateUser(Long userId, UserUpdateRequest userUpdateRequest) {
        var user = userRepository.findById(userId)
                .orElseThrow(() -> resourceNotFound(User.class));

        user.setFirstName(userUpdateRequest.firstName());
        user.setLastName(userUpdateRequest.lastName());

        return UserResponse.toUserResponse(userRepository.save(user));
    }

    @Transactional
    public void deleteUser(Long userId) {
        var user = userRepository.findById(userId)
                .orElseThrow(() -> resourceNotFound(User.class));
        userRepository.delete(user);
    }
}
