import React, { useState, useEffect } from "react";

const SessionTimer = () => {
    const [sessionStart] = useState(Date.now());
    const [sessionTime, setSessionTime] = useState(0);
    const [totalOperations, setTotalOperations] = useState(0);

    useEffect(() => {
        const interval = setInterval(() => {
            setSessionTime(Date.now() - sessionStart);
        }, 1000);

        return () => clearInterval(interval);
    }, [sessionStart]);

    const formatSessionTime = (ms) => {
        const totalSeconds = Math.floor(ms / 1000);
        const hours = Math.floor(totalSeconds / 3600);
        const minutes = Math.floor((totalSeconds % 3600) / 60);
        const seconds = totalSeconds % 60;

        if (hours > 0) {
            return `${hours}h ${minutes}m ${seconds}s`;
        } else if (minutes > 0) {
            return `${minutes}m ${seconds}s`;
        }
        return `${seconds}s`;
    };

    return (
        <div className="bg-gradient-to-r from-purple-50 to-pink-50 dark:from-purple-900 dark:to-pink-900 rounded-lg p-4 shadow-md">
            <div className="flex items-center justify-between">
                <div>
                    <div className="text-xs text-gray-600 dark:text-gray-300 mb-1">
                        Session Duration
                    </div>
                    <div className="text-2xl font-bold text-purple-700 dark:text-purple-200 font-mono">
                        {formatSessionTime(sessionTime)}
                    </div>
                </div>
                <div className="text-4xl">⏱️</div>
            </div>
        </div>
    );
};

export default SessionTimer;
