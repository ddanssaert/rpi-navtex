const STATIONS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'.split('');
const TYPES = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'.split('');

const FilterBar = ({ filters, toggleStation, toggleType }) => {
    return (
        <div className="glass-card mb-6 overflow-hidden">
            <div className="mb-4">
                <h3 className="text-secondary text-xs uppercase tracking-wider mb-2">Stations (B1)</h3>
                <div className="flex flex-wrap gap-2">
                    {STATIONS.map(id => (
                        <button
                            key={id}
                            onClick={() => toggleStation(id)}
                            className={`filter-chip ${filters.stations.includes(id) ? 'active' : ''}`}
                        >
                            {id}
                        </button>
                    ))}
                </div>
            </div>
            <div>
                <h3 className="text-secondary text-xs uppercase tracking-wider mb-2">Message Types (B2)</h3>
                <div className="flex flex-wrap gap-2">
                    {TYPES.map(id => (
                        <button
                            key={id}
                            onClick={() => toggleType(id)}
                            className={`filter-chip ${filters.types.includes(id) ? 'active' : ''}`}
                        >
                            {id}
                        </button>
                    ))}
                </div>
            </div>
        </div>
    );
};

export default FilterBar;
