export const getVelocityZone = (velocity) => {
  if (velocity < 0.3) return { label: 'Grinding', color: 'text-red-500', bg: 'bg-red-500/20' };
  if (velocity < 0.5) return { label: 'Strength', color: 'text-amber-500', bg: 'bg-amber-500/20' };
  if (velocity < 0.75) return { label: 'Power', color: 'text-green-500', bg: 'bg-green-500/20' };
  if (velocity < 1.0) return { label: 'Speed-Strength', color: 'text-blue-500', bg: 'bg-blue-500/20' };
  return { label: 'Speed', color: 'text-purple-500', bg: 'bg-purple-500/20' };
};
