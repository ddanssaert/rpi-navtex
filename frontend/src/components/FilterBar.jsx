import { useState } from 'react';
import { STATION_NAMES, MESSAGE_TYPE_NAMES, resolveStation, resolveType } from '../constants/navtex-codes';

const STATION_CODES = Object.keys(STATION_NAMES);
const TYPE_CODES = Object.keys(MESSAGE_TYPE_NAMES);

const FilterBar = ({ filters, toggleStation, toggleType }) => {
    const [stationsOpen, setStationsOpen] = useState(false);
    const [typesOpen, setTypesOpen] = useState(false);

    return (
        <div className="glass-card mb-6 overflow-hidden">
            <div className="mb-4">
                <button
                    className="w-full text-left text-secondary text-xs uppercase tracking-wider mb-2 flex items-center gap-1"
                    onClick={() => setStationsOpen(o => !o)}
                >
                    <span>{stationsOpen ? '▼' : '▶'}</span>
                    <span>Stations ({filters.stations.length} active)</span>
                </button>
                {stationsOpen && (
                    <div className="flex flex-col gap-1">
                        {STATION_CODES.map(code => {
                            const label = resolveStation(code);
                            return (
                                <label key={code} className="flex items-center gap-2 text-sm cursor-pointer">
                                    <input
                                        type="checkbox"
                                        className="accent-emerald-400"
                                        checked={filters.stations.includes(code)}
                                        onChange={() => toggleStation(code)}
                                        aria-label={label}
                                    />
                                    {label}
                                </label>
                            );
                        })}
                    </div>
                )}
            </div>
            <div>
                <button
                    className="w-full text-left text-secondary text-xs uppercase tracking-wider mb-2 flex items-center gap-1"
                    onClick={() => setTypesOpen(o => !o)}
                >
                    <span>{typesOpen ? '▼' : '▶'}</span>
                    <span>Message Types ({filters.types.length} active)</span>
                </button>
                {typesOpen && (
                    <div className="flex flex-col gap-1">
                        {TYPE_CODES.map(code => {
                            const label = resolveType(code);
                            return (
                                <label key={code} className="flex items-center gap-2 text-sm cursor-pointer">
                                    <input
                                        type="checkbox"
                                        className="accent-amber-400"
                                        checked={filters.types.includes(code)}
                                        onChange={() => toggleType(code)}
                                        aria-label={label}
                                    />
                                    {label}
                                </label>
                            );
                        })}
                    </div>
                )}
            </div>
        </div>
    );
};

export default FilterBar;
