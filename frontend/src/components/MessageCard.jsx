import { resolveStation, resolveType } from '../constants/navtex-codes';

const MessageCard = ({ message }) => {
    const date = new Date(message.timestamp).toLocaleString();

    return (
        <div className="glass-card mb-4">
            <div className="card-header">
                <span className="card-header-date">{date}</span>
                <span className="station-badge">{resolveStation(message.station_id)}</span>
                <span className="type-badge">{resolveType(message.message_type)}</span>
            </div>
            <div className="serial-id text-xs text-secondary mb-2">Serial: {message.serial_id}</div>
            <pre className="message-content text-sm whitespace-pre-wrap font-mono bg-black/20 p-2 rounded">
                {message.content}
            </pre>
        </div>
    );
};

export default MessageCard;
