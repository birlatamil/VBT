/**
 * RPE Engine - Deterministic RPE Projection
 * Estimates RPE based on velocity loss %, rep count, and velocity zones.
 */

function calculateRPE(velocityLossPct, repCount, avgVelocity) {
  let estimatedRPE = 0;

  // Base RPE based on velocity loss
  if (velocityLossPct < 5) {
    estimatedRPE = repCount <= 3 ? 6.5 : 7.0;
  } else if (velocityLossPct < 15) {
    estimatedRPE = repCount <= 5 ? 7.5 : 8.0;
  } else if (velocityLossPct < 25) {
    estimatedRPE = repCount <= 6 ? 8.5 : 9.0;
  } else {
    estimatedRPE = 9.5;
  }

  // Adjust based on absolute velocity (if grinding)
  if (avgVelocity < 0.3) {
    estimatedRPE = Math.max(estimatedRPE, 9.5); // Very close to failure
  } else if (avgVelocity > 0.6 && estimatedRPE > 8.0) {
    // Moving fast, shouldn't be high RPE
    estimatedRPE -= 1.0; 
  }

  // Cap RPE between 6 and 10
  return Math.min(Math.max(estimatedRPE, 6.0), 10.0);
}

module.exports = {
  calculateRPE
};
