/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        background: '#030712',
        surface: 'rgba(255,255,255,0.03)',
        border: 'rgba(255,255,255,0.06)',
        primary: {
          500: '#3B82F6',
          600: '#8B5CF6'
        },
        concentric: '#22C55E',
        eccentric: '#EF4444',
        warning: '#F59E0B'
      },
      fontFamily: {
        sans: ['Inter', 'sans-serif'],
      }
    },
  },
  plugins: [],
}
