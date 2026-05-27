export const formatTime = (seconds) => {
  if (!seconds && seconds !== 0) return '0.00s';
  return `${seconds.toFixed(2)}s`;
};

export const formatVelocity = (vel) => {
  if (!vel && vel !== 0) return '0.00';
  return vel.toFixed(2);
};
