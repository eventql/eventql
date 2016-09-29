# Zscale Android SDK
## Building
You can build an `.aar` package by executing:
```shell
./gradlew aR
```

## Usage

1. Add the eventql-android.aar as a new Module to your app.
2. Add dependencies to app/build.gradle
```gradle
dependencies {
    compile project(':eventql-android')
}
```
3. Add permisions to `app/src/main/AndroidManifest.xml`
```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
```
4. Add ZscaleApi to  your activity.
```java
ZscaleApi.getInstance(context.getApplicationContext(), "MyToken");
```
5. Track events
```java
try {
    final JSONObject event = new JSONObject();
    event.put("name", "test-event");
    event.put("device-id", "1234567890");
    event.put("device-time", System.currentTimeMillis());
    api.track("MyActivity", event);
} catch (JSONException e) {
    Log.e("MyActivity", "Something wrong with the json object");
}
```
