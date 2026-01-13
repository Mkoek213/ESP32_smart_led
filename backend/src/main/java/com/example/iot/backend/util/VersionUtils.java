package com.example.iot.backend.util;

import java.util.regex.Pattern;

public class VersionUtils {

    // Split by dot, hyphen, or underscore to handle various version formats
    private static final Pattern SPLIT_PATTERN = Pattern.compile("[._-]");

    /**
     * Checks if the target version is newer than the current version.
     * 
     * @param current The current version string (e.g., "1.0.0", "19.0")
     * @param target  The target version string to check (e.g., "1.0.1", "20.0")
     * @return true if target is strictly greater than current
     */
    public static boolean isNewer(String current, String target) {
        if (current == null || target == null) {
            return false;
        }

        if (current.equals(target)) {
            return false;
        }

        String[] currentParts = SPLIT_PATTERN.split(current);
        String[] targetParts = SPLIT_PATTERN.split(target);

        int length = Math.max(currentParts.length, targetParts.length);

        for (int i = 0; i < length; i++) {
            int v1 = i < currentParts.length ? parsePart(currentParts[i]) : 0;
            int v2 = i < targetParts.length ? parsePart(targetParts[i]) : 0;

            if (v1 < v2) {
                return true;
            }
            if (v1 > v2) {
                return false;
            }
        }

        return false;
    }

    private static int parsePart(String part) {
        try {
            // Remove any non-digit characters (e.g., "v1" -> "1", "rc2" -> "2")
            String cleanPart = part.replaceAll("\\D", "");
            return cleanPart.isEmpty() ? 0 : Integer.parseInt(cleanPart);
        } catch (NumberFormatException e) {
            return 0;
        }
    }
}
