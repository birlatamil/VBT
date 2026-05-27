/**
 * Velocity Analytics
 */

function calculateVelocityLoss(reps) {
  if (!reps || reps.length < 2) return 0;
  
  const firstRepVel = reps[0].avg_velocity;
  const lastRepVel = reps[reps.length - 1].avg_velocity;
  
  if (firstRepVel === 0) return 0;
  
  const loss = ((firstRepVel - lastRepVel) / firstRepVel) * 100;
  return Math.max(0, loss); // Don't return negative loss if they got faster
}

module.exports = {
  calculateVelocityLoss
};
