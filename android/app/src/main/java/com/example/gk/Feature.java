package com.example.gk;

/** One feature entry from the native catalog. */
public final class Feature {
    public final int id;
    public final String name;
    public final String domain;
    public final String status;

    public Feature(int id, String name, String domain, String status) {
        this.id = id;
        this.name = name;
        this.domain = domain;
        this.status = status;
    }

    public boolean isVerified() {
        return "VERIFIED".equals(status);
    }

    public boolean isImplemented() {
        return "IMPLEMENTED".equals(status);
    }
}
