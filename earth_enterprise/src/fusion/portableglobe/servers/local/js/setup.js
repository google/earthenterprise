// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// TODO: High-level file comment.

/**
 * Clears the hint from the field with the given id.
 * @param {string} id Id of field to be cleared.
 * @param {string} hint Hint in the field.
 */
function GEE_clearHint(id, hint) {
  item = document.getElementById(id);
  if (item.value == hint) {
    item.value = '';
  } else {
    item.select();
  }
}

/**
 * Restores the hint to the field with the given id.
 * @param {string} id Id of field to be cleared.
 * @param {string} hint Hint to be restored.
 */
function GEE_restoreHint(id, hint) {
  item = document.getElementById(id);
  if (item.value == '') {
    item.value = hint;
  }
}

/**
 * Redirects user to a generated glr file.
 * @return {boolean} false.
 */
function GEE_getGlr() {
  return GEE_redirect('/share.glr');
}

/**
 * Creates a new glr file.
 * @return {boolean} false.
 */
function GEE_addGlr() {
  name = document.forms.add_glr_form.name.value;
  if (name == '... no spaces please') {
    alert('Please enter a name for the glr.');
    return false;
  }

  server = document.forms.add_glr_form.server.value;
  if (server == '... enter server') {
    alert('Please enter a server.');
    return false;
  }

  key = document.forms.add_glr_form.key.value;
  if (key == '... enter key') {
    alert('Please enter a key.');
    return false;
  }

  port = document.forms.add_glr_form.port.value;
  document.location = '/?server=' + server + '&key=' + key +
                      '&port=' + port + '&name=' + name + '&cmd=add_glr';
  return false;
}

/**
 * Redirects user to given url.
 * @param {string} url Url to redirect to.
 * @return {boolean} false.
 */
function GEE_redirect(url) {
  document.location = url;
  return false;
}

