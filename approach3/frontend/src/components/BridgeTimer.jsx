import React, { useState, useEffect } from "react";

const BridgeTimer = ({ bridgeState, currentState }) => {
    const [elapsedTime, setElapsedTime] = useState(0);
    const [startTime, setStartTime] = useState(null);
    const [timerActive, setTimerActive] = useState(false);

    // Reset timer when bridge state changes
    useEffect(() => {
        setStartTime(Date.now());
        setElapsedTime(0);
        setTimerActive(true);
    }, [bridgeState, currentState]);

    // Update elapsed time every 100ms
    useEffect(() => {
        if (!timerActive || !startTime) return;

        const interval = setInterval(() => {
            setElapsedTime(Date.now() - startTime);
        }, 100);

        return () => clearInterval(interval);
    }, [timerActive, startTime]);

    const formatTime = (ms) => {
        const totalSeconds = Math.floor(ms / 1000);
        const minutes = Math.floor(totalSeconds / 60);
        const seconds = totalSeconds % 60;
        const deciseconds = Math.floor((ms % 1000) / 100);

        if (minutes > 0) {
            return `${minutes}m ${seconds}.${deciseconds}s`;
        }
        return `${seconds}.${deciseconds}s`;
    };

    const getTimerLabel = () => {
        if (currentState >= 3 && currentState <= 5) {
            return "Bridge Open";
        } else if (currentState >= 0 && currentState <= 2) {
            return "Bridge Closed";
        } else if (currentState === 7) {
            return "Closing";
        } else if (currentState === 8) {
            return "Closed";
        }
        return "Bridge Status";
    };

    const getTimerColor = () => {
        if (currentState >= 3 && currentState <= 5) {
            return "text-green-600 dark:text-green-400";
        } else if (currentState === 7) {
            return "text-yellow-600 dark:text-yellow-400";
        }
        return "text-blue-600 dark:text-blue-400";
    };

    const getTimerIcon = () => {
        if (currentState >= 3 && currentState <= 5) {
            return "🌉";
        } else if (currentState === 7) {
            return "⏳";
        }
        return "🔒";
    };

    return (
        <div className="bg-gradient-to-br from-gray-50 to-gray-100 dark:from-gray-800 dark:to-gray-900 rounded-lg p-6 shadow-lg border border-gray-200 dark:border-gray-700">
            <div className="flex items-center justify-between mb-4">
                <div className="flex items-center gap-3">
                    <span className="text-3xl">{getTimerIcon()}</span>
                    <h3 className="text-lg font-semibold text-gray-700 dark:text-gray-200">
                        {getTimerLabel()}
                    </h3>
                </div>
                <div
                    className={`text-xs px-3 py-1 rounded-full ${
                        timerActive
                            ? "bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-200"
                            : "bg-gray-100 text-gray-800 dark:bg-gray-700 dark:text-gray-200"
                    }`}
                >
                    {timerActive ? "● LIVE" : "○ IDLE"}
                </div>
            </div>

            <div className="flex flex-col items-center py-4">
                <div
                    className={`text-5xl font-bold font-mono ${getTimerColor()} tracking-wider`}
                >
                    {formatTime(elapsedTime)}
                </div>
                <div className="text-sm text-gray-500 dark:text-gray-400 mt-2">
                    Time Elapsed
                </div>
            </div>

            {/* Progress bar for state transitions */}
            <div className="mt-4">
                <div className="flex justify-between text-xs text-gray-500 dark:text-gray-400 mb-1">
                    <span>Progress</span>
                    <span>State {currentState}</span>
                </div>
                <div className="w-full bg-gray-200 dark:bg-gray-700 rounded-full h-2">
                    <div
                        className={`h-2 rounded-full transition-all duration-1000 ${
                            currentState >= 3 && currentState <= 5
                                ? "bg-green-500"
                                : currentState === 7
                                ? "bg-yellow-500"
                                : "bg-blue-500"
                        }`}
                        style={{
                            width: `${Math.min(
                                (elapsedTime / 30000) * 100,
                                100
                            )}%`,
                        }}
                    />
                </div>
            </div>
        </div>
    );
};

export default BridgeTimer;
