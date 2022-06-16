!( () => {
  /**
   * Setup
   */
  document.addEventListener( "DOMContentLoaded", event => {
    document.querySelectorAll( "button[data-section]" ).forEach( button => {
      button.addEventListener( "click", event => {
        event.preventDefault();
        const section = button.getAttribute( "data-section" );
        document.body.className = "";
        document.body.classList.add( "loading", section );
        window.dispatchEvent( new Event( `load-${section}` ) );
      } );
    } );
    document.body.classList.add( "start" );
    window.dispatchEvent( new Event( "load-start" ) );
  } );

  /**
   * Notification
   */
   const aside = ( type, message ) => {
    const aside = document.querySelector( "aside" );
    aside.className = type;
    aside.textContent = message;
    document.body.classList.add( "has-aside" );
  };

  /**
   * Error
   */
  const errorHandler = message => {
    document.body.classList.remove( "loading" );
    aside( "error", message );
  };

  /**
   * Start View
   */
  window.addEventListener( "load-start", event => {
    fetch( "/config" )
      .then( response => response.json() )
      .then( config => {
        document.querySelector( "[data-section=firmware]" )
          .disabled = !config.FW_ALLOW_OTA_UPDATE;
        document.body.classList.remove( "loading" );
      } )
      .catch( errorHandler );
  } );

  /**
   * Status View
   */
  window.addEventListener( "load-status", event => {
    fetch( "/status" )
      .then( response => response.json() )
      .then( status => {
        for( var key in status ) {
          document.querySelectorAll( `#status #${key}` )
            .forEach( field => field.innerText = status[ key ] );
        }
        document.body.classList.remove( "loading" );
      } )
      .catch( errorHandler );
  } );

  /**
   * Settings View
   */
  window.addEventListener( "load-settings", event => {
    fetch( "/config" )
      .then( response => response.json() )
      .then( config => {
        for( var key in config ) {
          document.querySelectorAll( `#settings #${key}` )
            .forEach( field => field.value = config[ key ] );
        }
        document.body.classList.remove( "loading" );
      } )
      .catch( errorHandler );
    document.querySelector( "#update-settings" )
      .addEventListener( "submit", event => {
        event.preventDefault();
        document.body.classList.add( "loading" );
        const config = new FormData( event.target );
        fetch( "/config", {
          method: "POST",
          body: Object.fromEntries( config.entries() )
        } )
          .then( response => response.json() )
          .then( () => document.body.classList.remove( "loading" ) )
          .catch( errorHandler );
      } );
  } );

  /**
   * Reset View
   */
  window.addEventListener( "load-reset", event => {
    document.body.classList.remove( "loading" );
    document.querySelector( "#confirm-reset" )
      .addEventListener( "click", event => {
        event.preventDefault();
        document.body.classList.add( "loading" );
        fetch( "/reset" )
          .then( response => {
            if( response.ok ) {
              setTimeout( () => {
                window.location.reload();
              }, 5000 );
            }
          } )
          .catch( errorHandler );
      } );
  } );

  /**
   * Reboot View
   */
  window.addEventListener( "load-reboot", event => {
    document.body.classList.remove( "loading" );
    document.querySelector( "#confirm-reboot" )
      .addEventListener( "click", event => {
        event.preventDefault();
        document.body.classList.add( "loading" );
        fetch( "/reboot" )
          .then( response => {
            if( response.ok ) {
              setTimeout( () => {
                window.location.reload();
              }, 5000 );
            }
          } )
          .catch( errorHandler );
      } );
  } );

  /**
   * Firmware View
   */
  window.addEventListener( "load-firmware", event => {
    fetch( "/firmware" )
      .then( response => response.json() )
      .then( firmware => {
        document.body.classList.remove( "loading" );
        document.querySelector( "#firmware-version" ).textContent = firmware.version;
      } )
      .catch( errorHandler );
    document.querySelector( "#update-firmware" )
      .addEventListener( "submit", event => {
        event.preventDefault();
        document.body.classList.add( "loading" );
        fetch( "/firmware", {
          method: "POST",
          body: new FormData( event.target )
        } )
          .then( response => {
            if( response.ok ) {
              setTimeout( () => {
                window.location.reload();
              }, 30000 );
            }
          } )
          .catch( errorHandler );
      } );
  } );
} )();