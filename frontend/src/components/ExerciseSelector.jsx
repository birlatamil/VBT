import React from 'react';
import { socket } from '../services/socket';
import { useSessionStore } from '../store/sessionStore';
import { clsx } from 'clsx';
import { twMerge } from 'tailwind-merge';

function cn(...inputs) {
  return twMerge(clsx(inputs));
}

const exercises = [
  { id: 'squat', label: 'Squat' },
  { id: 'bench', label: 'Bench' },
  { id: 'deadlift', label: 'Deadlift' }
];

export function ExerciseSelector() {
  const { exercise, deviceId, sessionStatus } = useSessionStore();
  const isStarted = sessionStatus === 'started';

  const handleChange = (newExercise) => {
    if (isStarted) return; // Don't allow changing exercise mid-set
    socket.emit('exercise:change', { deviceId, exercise: newExercise });
    useSessionStore.getState().setExercise(newExercise);
  };

  return (
    <div className="flex bg-surface p-1 rounded-xl border border-border">
      {exercises.map((ex) => {
        const isActive = exercise === ex.id;
        return (
          <button
            key={ex.id}
            onClick={() => handleChange(ex.id)}
            disabled={isStarted}
            className={cn(
              "px-6 py-2 rounded-lg text-sm font-medium transition-all",
              isActive ? "bg-primary-600 text-white shadow-md" : "text-gray-400 hover:text-white hover:bg-white/5",
              isStarted && !isActive && "opacity-50 cursor-not-allowed"
            )}
          >
            {ex.label}
          </button>
        );
      })}
    </div>
  );
}
