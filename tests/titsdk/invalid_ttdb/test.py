from titsdk import Error, open_storage

try:
  # Our current script definitely is not a valid `.ttdb` file.
  open_storage(__file__)
except Error as e:
  print(f"Expected error: {e}")
