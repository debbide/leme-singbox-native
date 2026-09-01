{
  "targets": [
    {
      "target_name": "leme_native",
      "sources": ["leme_native.c"],
      "defines": ["NAPI_VERSION=8"],
      "conditions": [
        ["OS!='win'", {
          "libraries": ["-ldl"]
        }]
      ]
    }
  ]
}