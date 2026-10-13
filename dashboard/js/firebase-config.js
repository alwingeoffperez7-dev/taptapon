// Shared Firebase setup, imported by every dashboard and simulator page.
// This web config is public by design (it's visible in any browser anyway) and safe to commit.
// Security comes from firebase/database.rules.json, not from hiding these values.
import { initializeApp } from "https://www.gstatic.com/firebasejs/10.12.2/firebase-app.js";
import { getDatabase } from "https://www.gstatic.com/firebasejs/10.12.2/firebase-database.js";
import { getAuth } from "https://www.gstatic.com/firebasejs/10.12.2/firebase-auth.js";

// For Firebase JS SDK v7.20.0 and later, measurementId is optional
const firebaseConfig = {
  apiKey: "AIzaSyAqyVsTLL5wjkgMb4FPrQoZE2EoxO6Xk7k",
  authDomain: "taptapon.firebaseapp.com",
  databaseURL:
    "https://taptapon-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "taptapon",
  storageBucket: "taptapon.firebasestorage.app",
  messagingSenderId: "724535832482",
  appId: "1:724535832482:web:b786b6369b21ea291accfb",
  measurementId: "G-0688D8ETYR",
};

export const app = initializeApp(firebaseConfig);
export const db = getDatabase(app);
export const auth = getAuth(app);
