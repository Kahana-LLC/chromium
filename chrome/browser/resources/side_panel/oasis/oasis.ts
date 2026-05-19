// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const messagesEl = document.getElementById('messages') as HTMLElement;
const inputEl = document.getElementById('user-input') as HTMLInputElement;
const sendBtn = document.getElementById('send-btn') as HTMLElement;

function appendMessage(text: string, role: 'user'|'assistant'): void {
  const el = document.createElement('div');
  el.className = 'message ' + role;
  el.textContent = text;
  messagesEl.appendChild(el);
  messagesEl.scrollTop = messagesEl.scrollHeight;
}

// TODO: Replace the placeholder response with a real API call.
function handleUserMessage(message: string): void {
  appendMessage('(API not yet connected) You said: ' + message, 'assistant');
}

function submitMessage(): void {
  const text = inputEl.value.trim();
  if (!text) {
    return;
  }
  appendMessage(text, 'user');
  inputEl.value = '';
  handleUserMessage(text);
}

sendBtn.addEventListener('click', submitMessage);
inputEl.addEventListener('keydown', (e: KeyboardEvent) => {
  if (e.key === 'Enter' && !e.shiftKey) {
    e.preventDefault();
    submitMessage();
  }
});
