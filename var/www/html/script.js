const cursor = document.querySelector('.firework-cursor');
const fireworkLayer = document.getElementById('firework-layer');

document.addEventListener('mousemove', (e) => {
  cursor.style.left = e.clientX + 'px';
  cursor.style.top = e.clientY + 'px';
  cursor.style.display = 'block';

  createFirework(e.clientX, e.clientY);
});

function createFirework(x, y) {
  const particles = 12;

  for (let i = 0; i < particles; i++) {
    const particle = document.createElement('div');
    particle.classList.add('particle');
    particle.style.left = x + 'px';
    particle.style.top = y + 'px';

    const hue = Math.floor(Math.random() * 360);

    const angle = (Math.PI * 2 * i) / particles;
    const radius = 60 + Math.random() * 20;
    const dx = Math.cos(angle) * radius;
    const dy = Math.sin(angle) * radius;

    particle.style.setProperty('--x', `${dx}px`);
    particle.style.setProperty('--y', `${dy}px`);
    particle.style.setProperty('--hue', hue);

    fireworkLayer.appendChild(particle);

    const duration = document.body.classList.contains('party-mode') ? 1200 : 600;

    setTimeout(() => {
      particle.remove();
    }, duration);
  }
}
