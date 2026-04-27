import { useState, useEffect, useRef } from 'react';
import { syncFiltersToServer } from '../utils/pushSync';

const STORAGE_KEY = 'navtex_filters';

const useFilters = () => {
    const [filters, setFilters] = useState(() => {
        const saved = localStorage.getItem(STORAGE_KEY);
        return saved ? JSON.parse(saved) : { stations: [], types: [] };
    });
    const isFirstRender = useRef(true);

    useEffect(() => {
        localStorage.setItem(STORAGE_KEY, JSON.stringify(filters));
    }, [filters]);

    useEffect(() => {
        if (isFirstRender.current) {
            isFirstRender.current = false;
            return;
        }
        const timer = setTimeout(() => { syncFiltersToServer(filters); }, 800);
        return () => clearTimeout(timer);
    }, [filters]);

    const toggleStation = (id) => {
        setFilters(prev => ({
            ...prev,
            stations: prev.stations.includes(id)
                ? prev.stations.filter(s => s !== id)
                : [...prev.stations, id]
        }));
    };

    const toggleType = (id) => {
        setFilters(prev => ({
            ...prev,
            types: prev.types.includes(id)
                ? prev.types.filter(t => t !== id)
                : [...prev.types, id]
        }));
    };

    const isFiltered = (msg) => {
        const stationMatch = filters.stations.length === 0 || filters.stations.includes(msg.station_id);
        const typeMatch = filters.types.length === 0 || filters.types.includes(msg.message_type);
        return stationMatch && typeMatch;
    };

    return { filters, toggleStation, toggleType, isFiltered };
};

export default useFilters;
