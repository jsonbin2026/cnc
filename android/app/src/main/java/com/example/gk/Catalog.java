package com.example.gk;

import java.util.ArrayList;
import java.util.List;

/**
 * In-memory catalog, populated once from the native engine.
 * The JNI bridge returns lines of "id|name|domain|status".
 */
public final class Catalog {

    private static Catalog instance;

    private final List<Feature> features = new ArrayList<>();
    private final List<Section> sections = Section.all();

    private Catalog(String dump) {
        if (dump == null) {
            return;
        }
        for (String line : dump.split("\n")) {
            if (line.isEmpty()) {
                continue;
            }
            String[] parts = line.split("\\|", 4);
            if (parts.length < 4) {
                continue;
            }
            try {
                features.add(new Feature(
                        Integer.parseInt(parts[0].trim()),
                        parts[1], parts[2], parts[3]));
            } catch (NumberFormatException ignored) {
                // skip malformed line
            }
        }
    }

    public static synchronized Catalog get() {
        if (instance == null) {
            instance = new Catalog(MainActivity.nativeCatalogDump());
        }
        return instance;
    }

    public static synchronized void reset() {
        instance = null;
    }

    public List<Section> sections() {
        return sections;
    }

    public List<Feature> all() {
        return features;
    }

    public int total() {
        return features.size();
    }

    /** Features belonging to a module, by id range. */
    public List<Feature> featuresOf(Section section) {
        List<Feature> out = new ArrayList<>();
        for (Feature f : features) {
            if (section.contains(f.id)) {
                out.add(f);
            }
        }
        return out;
    }

    public Feature byId(int id) {
        for (Feature f : features) {
            if (f.id == id) {
                return f;
            }
        }
        return null;
    }
}
