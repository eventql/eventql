'use strict'

class AuthProvider {
  constructor() {
    this.database = null;
    this.auth_token = null;
    this.user = null;
    this.password = null;
  }

  setDatabase(database) {
    this.database = database;
  }

  hasDatabase() {
    return this.database !== null;
  }

  setAuthToken(auth_token) {
    this.auth_token = auth_token;
  }

  hasAuthToken() {
    return this.auth_token !== null;
  }

  setUser(user) {
    this.user = user;
  }

  hasUser() {
    return this.user !== null;
  }

  setPassword(password) {
    this.password = password;
  }

  hasPassword() {
    return this.password !== null;
  }

}

module.exports = AuthProvider;

