async function postJSON(url, data){
  const res = await fetch(url, {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)});
  const json = await res.json().catch(()=>({ok:false,msg:'invalid response'}));
  return {status: res.status, json};
}

const signupBtn = document.getElementById('signup');
if (signupBtn) {
  signupBtn.addEventListener('click', async ()=>{
    const usernameEl = document.getElementById('su-username');
    const passwordEl = document.getElementById('su-password');
    const el = document.getElementById('su-msg');
    const username = usernameEl ? usernameEl.value.trim() : '';
    const password = passwordEl ? passwordEl.value : '';
    const {status,json} = await postJSON('/api/signup',{username,password});
    if (el) {
      el.textContent = json.msg || (json.error||'unknown');
      el.className = 'msg' + (json.ok ? '' : ' error');
    }
  });
}

const signinBtn = document.getElementById('signin');
if (signinBtn) {
  signinBtn.addEventListener('click', async ()=>{
    const usernameEl = document.getElementById('si-username');
    const passwordEl = document.getElementById('si-password');
    const el = document.getElementById('si-msg');
    const username = usernameEl ? usernameEl.value.trim() : '';
    const password = passwordEl ? passwordEl.value : '';
    const {status,json} = await postJSON('/api/signin',{username,password});
    if (el) {
      el.textContent = json.msg || (json.error||'unknown');
      el.className = 'msg' + (json.ok ? '' : ' error');
    }
  });
}

const exitBtn = document.getElementById('exit');
if (exitBtn) {
  exitBtn.addEventListener('click', ()=>{
    const suu = document.getElementById('su-username'); if (suu) suu.value='';
    const sup = document.getElementById('su-password'); if (sup) sup.value='';
    const siu = document.getElementById('si-username'); if (siu) siu.value='';
    const sip = document.getElementById('si-password'); if (sip) sip.value='';
    const sum = document.getElementById('su-msg'); if (sum) sum.textContent='';
    const sim = document.getElementById('si-msg'); if (sim) sim.textContent='';
    const info = document.getElementById('info'); if (info) info.textContent='Exited. You can signup or signin again.';
  });
}
