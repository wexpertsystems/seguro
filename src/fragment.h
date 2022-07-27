//! @file fragment.h
//!
//! Splits large events (which exceed the maximum size limit of the database) 
//! into multiple fragments for storage in FoundationDB key/value pairs.