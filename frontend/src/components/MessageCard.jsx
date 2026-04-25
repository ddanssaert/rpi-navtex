const MessageCard = ({ message }) => {
    const date = new Date(message.timestamp).toLocaleString();

    return (
        <div className="glass-card mb-4">
            <div className="flex justify-between items-start mb-2">
                <div>
                    <span className="station-badge">{message.station_id}</span>
                    <span className="type-badge ml-2">{message.message_type}</span>
                </div>
                <span className="text-secondary text-sm">{date}</span>
            </div>
            <div className="serial-id text-xs text-secondary mb-2">Serial: {message.serial_id}</div>
            <pre className="message-content text-sm whitespace-pre-wrap font-mono bg-black/20 p-2 rounded">
                {message.content}
            </pre>
        </div>
    );
};

export default MessageCard;
