const select = document.getElementById('tracking-speed');
const container = document.getElementById('input-container');
const input = document.getElementById('option-input');
const button = document.getElementById('save-button');

select.addEventListener('change', () => {
  if (select.value === 'custom') {
    container.style.display = 'block';
  } else {
    container.style.display = 'none';
  }
});