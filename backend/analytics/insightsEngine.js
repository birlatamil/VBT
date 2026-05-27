/**
 * Insights Engine - Generates deterministic text insights
 */

function generateInsights(sessionReps, velocityLossPct, rpe) {
  const insights = [];

  if (sessionReps.length === 0) return insights;

  const firstRep = sessionReps[0];
  const lastRep = sessionReps[sessionReps.length - 1];

  // 1. Velocity Drop Insight
  if (velocityLossPct > 20) {
    insights.push({
      type: 'warning',
      message: `Significant velocity drop (${velocityLossPct.toFixed(1)}%). Fatigue accumulation is high. Consider ending the set or lowering the weight.`
    });
  } else if (velocityLossPct < 5 && sessionReps.length >= 3) {
    insights.push({
      type: 'success',
      message: `Consistent power output. Velocity loss is minimal (${velocityLossPct.toFixed(1)}%). You have more in the tank.`
    });
  }

  // 2. Absolute Velocity Insight
  if (firstRep.avg_velocity > 0.75) {
    insights.push({
      type: 'info',
      message: 'Great bar speed on the initial rep! You are in the Speed-Strength zone.'
    });
  } else if (lastRep.avg_velocity < 0.3) {
    insights.push({
      type: 'warning',
      message: 'Bar speed is grinding (< 0.3 m/s). Approaching muscular failure.'
    });
  }

  // 3. RPE Insight
  if (rpe >= 9.5) {
    insights.push({
      type: 'danger',
      message: 'Projected RPE is 9.5+. Rest well before the next set.'
    });
  }

  // 4. Technique/Consistency (StdDev of concentric times)
  if (sessionReps.length > 3) {
    const avgConTime = sessionReps.reduce((sum, r) => sum + r.con_time, 0) / sessionReps.length;
    const stdDevConTime = Math.sqrt(sessionReps.reduce((sum, r) => sum + Math.pow(r.con_time - avgConTime, 2), 0) / sessionReps.length);
    
    if (stdDevConTime < 0.05) {
      insights.push({
        type: 'success',
        message: 'Excellent technique consistency. Concentric timing is highly stable.'
      });
    }
  }

  return insights;
}

module.exports = {
  generateInsights
};
